/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWaylandCompositor module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwaylandpointer.h"
#include "qwaylandpointer_p.h"
#include <QtWaylandCompositor/QWaylandClient>
#include <QtWaylandCompositor/QWaylandCompositor>

QT_BEGIN_NAMESPACE

QWaylandSurfaceRole QWaylandPointerPrivate::s_role("wl_pointer");

QWaylandPointerPrivate::QWaylandPointerPrivate(QWaylandPointer *pointer, QWaylandSeat *seat)
    : QObjectPrivate()
    , wl_pointer()
    , seat(seat)
    , output()
    , hasSentEnter(false)
    , enterSerial(0)
    , buttonCount()
{
    Q_UNUSED(pointer);
}

const QList<QtWaylandServer::wl_pointer::Resource *> QWaylandPointerPrivate::pointerResourcesForFocusedSurface() const
{
    if (!seat->mouseFocus())
        return {};

    return resourceMap().values(seat->mouseFocus()->surfaceResource()->client);
}

uint QWaylandPointerPrivate::sendButton(Qt::MouseButton button, uint32_t state)
{
    Q_Q(QWaylandPointer);
    uint32_t time = compositor()->currentTimeMsecs();
    uint32_t serial = compositor()->nextSerial();
    for (auto resource : pointerResourcesForFocusedSurface())
        send_button(resource->handle, serial, time, q->toWaylandButton(button), state);
    return serial;
}

void QWaylandPointerPrivate::pointer_release(wl_pointer::Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

void QWaylandPointerPrivate::pointer_set_cursor(wl_pointer::Resource *resource, uint32_t serial, wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y)
{
    Q_UNUSED(resource);
    Q_UNUSED(serial);

    if (!surface) {
        seat->cursorSurfaceRequest(Q_NULLPTR, 0, 0);
        return;
    }

    QWaylandSurface *s = QWaylandSurface::fromResource(surface);
    // XXX FIXME
    // The role concept was formalized in wayland 1.7, so that release adds one error
    // code for each interface that implements a role, and we are supposed to pass here
    // the newly constructed resource and the correct error code so that if setting the
    // role fails, a proper error can be sent to the client.
    // However we're still using wayland 1.4, which doesn't have interface specific role
    // errors, so the best we can do is to use wl_display's object_id error.
    wl_resource *displayRes = wl_client_get_object(resource->client(), 1);
    if (s->setRole(&QWaylandPointerPrivate::s_role, displayRes, WL_DISPLAY_ERROR_INVALID_OBJECT)) {
        s->markAsCursorSurface(true);
        seat->cursorSurfaceRequest(s, hotspot_x, hotspot_y);
    }
}

/*!
 * \class QWaylandPointer
 * \inmodule QtWaylandCompositor
 * \since 5.8
 * \brief The QWaylandPointer class represents a pointer device.
 *
 * This class provides access to the pointer device in a QWaylandSeat. It corresponds to
 * the Wayland interface wl_pointer.
 */

/*!
 * Constructs a QWaylandPointer for the given \a seat and with the given \a parent.
 */
QWaylandPointer::QWaylandPointer(QWaylandSeat *seat, QObject *parent)
    : QWaylandObject(* new QWaylandPointerPrivate(this, seat), parent)
{
    connect(&d_func()->focusDestroyListener, &QWaylandDestroyListener::fired, this, &QWaylandPointer::focusDestroyed);
    connect(seat, &QWaylandSeat::mouseFocusChanged, this, &QWaylandPointer::pointerFocusChanged);
}

/*!
 * Returns the input device for this QWaylandPointer.
 */
QWaylandSeat *QWaylandPointer::seat() const
{
    Q_D(const QWaylandPointer);
    return d->seat;
}

/*!
 * Returns the compositor for this QWaylandPointer.
 */
QWaylandCompositor *QWaylandPointer::compositor() const
{
    Q_D(const QWaylandPointer);
    return d->compositor();
}

/*!
 * Returns the output for this QWaylandPointer.
 */
QWaylandOutput *QWaylandPointer::output() const
{
    Q_D(const QWaylandPointer);
    return d->output;
}

/*!
 * Sets the output for this QWaylandPointer to \a output.
 */
void QWaylandPointer::setOutput(QWaylandOutput *output)
{
    Q_D(QWaylandPointer);
    if (d->output == output) return;
    d->output = output;
    outputChanged();
}

/*!
 * Sends a mouse press event for \a button to the view currently holding mouse focus.
 *
 * Returns the serial number of the press event.
 */
uint QWaylandPointer::sendMousePressEvent(Qt::MouseButton button)
{
    Q_D(QWaylandPointer);
    d->buttonCount++;
    uint serial = 0;

    if (d->seat->mouseFocus())
         serial = d->sendButton(button, WL_POINTER_BUTTON_STATE_PRESSED);

    if (d->buttonCount == 1) {
        emit buttonPressedChanged();
    }

    return serial;
}

/*!
 * Sends a mouse release event for \a button to the view currently holding mouse focus.
 *
 * Returns the serial number of the release event.
 */
uint QWaylandPointer::sendMouseReleaseEvent(Qt::MouseButton button)
{
    Q_D(QWaylandPointer);
    d->buttonCount--;
    uint serial = 0;

    if (d->seat->mouseFocus())
         serial = d->sendButton(button, WL_POINTER_BUTTON_STATE_RELEASED);

    if (d->buttonCount == 0)
        emit buttonPressedChanged();

    return serial;
}

/*!
 * Sets the current mouse focus to \a view and sends a mouse move event to it with the
 * local position \a localPos and output space position \a outputSpacePos.
 */
void QWaylandPointer::sendMouseMoveEvent(QWaylandView *view, const QPointF &localPos, const QPointF &outputSpacePos)
{
    Q_D(QWaylandPointer);
    if (view && (!view->surface() || view->surface()->isCursorSurface()))
        view = Q_NULLPTR;
    d->seat->setMouseFocus(view);
    d->localPosition = localPos;
    d->spacePosition = outputSpacePos;

    //we adjust if the mouse position is on the edge
    //to work around Qt's event propagation
    if (view && view->surface()) {
        QSizeF size(view->surface()->size());
        if (d->localPosition.x() ==  size.width())
            d->localPosition.rx() -= 0.01;

        if (d->localPosition.y() == size.height())
            d->localPosition.ry() -= 0.01;
    }

    if (!d->hasSentEnter) {
        d->enterSerial = d->compositor()->nextSerial();
        QWaylandKeyboard *keyboard = d->seat->keyboard();
        if (keyboard)
            keyboard->sendKeyModifiers(view->surface()->client(), d->enterSerial);
        for (auto resource : d->pointerResourcesForFocusedSurface()) {
            d->send_enter(resource->handle, d->enterSerial, view->surface()->resource(),
                          wl_fixed_from_double(d->localPosition.x()),
                          wl_fixed_from_double(d->localPosition.y()));
        }
        d->focusDestroyListener.listenForDestruction(view->surface()->resource());
        d->hasSentEnter = true;
    }

    if (view && view->output())
        setOutput(view->output());

    uint32_t time = d->compositor()->currentTimeMsecs();

    if (d->seat->mouseFocus()) {
        wl_fixed_t x = wl_fixed_from_double(currentLocalPosition().x());
        wl_fixed_t y = wl_fixed_from_double(currentLocalPosition().y());
        for (auto resource : d->pointerResourcesForFocusedSurface())
            wl_pointer_send_motion(resource->handle, time, x, y);
    }
}

/*!
 * Sends a mouse wheel event with the given \a orientation and \a delta to the view that currently holds mouse focus.
 */
void QWaylandPointer::sendMouseWheelEvent(Qt::Orientation orientation, int delta)
{
    Q_D(QWaylandPointer);
    if (!d->seat->mouseFocus())
        return;

    uint32_t time = d->compositor()->currentTimeMsecs();
    uint32_t axis = orientation == Qt::Horizontal ? WL_POINTER_AXIS_HORIZONTAL_SCROLL
                                                  : WL_POINTER_AXIS_VERTICAL_SCROLL;

    for (auto resource : d->pointerResourcesForFocusedSurface())
        d->send_axis(resource->handle, time, axis, wl_fixed_from_int(-delta / 12));
}

/*!
 * Returns the view that currently holds mouse focus.
 */
QWaylandView *QWaylandPointer::mouseFocus() const
{
    Q_D(const QWaylandPointer);
    return d->seat->mouseFocus();
}

/*!
 * Returns the current local position of the QWaylandPointer.
 */
QPointF QWaylandPointer::currentLocalPosition() const
{
    Q_D(const QWaylandPointer);
    return d->localPosition;
}

/*!
 * Returns the current output space position of the QWaylandPointer.
 */
QPointF QWaylandPointer::currentSpacePosition() const
{
    Q_D(const QWaylandPointer);
    return d->spacePosition;
}

/*!
 * Returns true if any button is currently pressed. Otherwise returns false.
 */
bool QWaylandPointer::isButtonPressed() const
{
    Q_D(const QWaylandPointer);
    return d->buttonCount > 0;
}

/*!
 * \internal
 */
void QWaylandPointer::addClient(QWaylandClient *client, uint32_t id, uint32_t version)
{
    Q_D(QWaylandPointer);
    wl_resource *resource = d->add(client->client(), id, qMin<uint32_t>(QtWaylandServer::wl_pointer::interfaceVersion(), version))->handle;
    QWaylandView *focus = d->seat->mouseFocus();
    if (focus && client == focus->surface()->client()) {
        d->send_enter(resource, d->enterSerial, focus->surfaceResource(),
                      wl_fixed_from_double(d->localPosition.x()),
                      wl_fixed_from_double(d->localPosition.y()));
    }
}

/*!
 * Returns a Wayland resource for this QWaylandPointer.
 *
 * This API doesn't actually make sense, since there may be many pointer resources per client
 * It's here for compatibility reasons.
 */
struct wl_resource *QWaylandPointer::focusResource() const
{
    Q_D(const QWaylandPointer);
    QWaylandView *focus = d->seat->mouseFocus();
    if (!focus)
        return nullptr;

    // Just return the first resource we can find.
    return d->resourceMap().value(focus->surface()->waylandClient())->handle;
}

/*!
 * \internal
 */
uint QWaylandPointer::sendButton(struct wl_resource *resource, uint32_t time, Qt::MouseButton button, uint32_t state)
{
    // This method is here for compatibility reasons only, since it usually doesn't make sense to
    // send button events to just one of the pointer resources for a client.
    Q_D(QWaylandPointer);
    uint32_t serial = d->compositor()->nextSerial();
    d->send_button(resource, serial, time, toWaylandButton(button), state);
    return serial;
}

/*!
 * \internal
 */
uint32_t QWaylandPointer::toWaylandButton(Qt::MouseButton button)
{
#ifndef BTN_LEFT
    uint32_t BTN_LEFT = 0x110;
#endif
    // the range of valid buttons (evdev module) is from 0x110
    // through 0x11f. 0x120 is the first 'Joystick' button.
    switch (button) {
    case Qt::LeftButton: return BTN_LEFT;
    case Qt::RightButton: return uint32_t(0x111);
    case Qt::MiddleButton: return uint32_t(0x112);
    case Qt::ExtraButton1: return uint32_t(0x113);  // AKA Qt::BackButton, Qt::XButton1
    case Qt::ExtraButton2: return uint32_t(0x114);  // AKA Qt::ForwardButton, Qt::XButton2
    case Qt::ExtraButton3: return uint32_t(0x115);
    case Qt::ExtraButton4: return uint32_t(0x116);
    case Qt::ExtraButton5: return uint32_t(0x117);
    case Qt::ExtraButton6: return uint32_t(0x118);
    case Qt::ExtraButton7: return uint32_t(0x119);
    case Qt::ExtraButton8: return uint32_t(0x11a);
    case Qt::ExtraButton9: return uint32_t(0x11b);
    case Qt::ExtraButton10: return uint32_t(0x11c);
    case Qt::ExtraButton11: return uint32_t(0x11d);
    case Qt::ExtraButton12: return uint32_t(0x11e);
    case Qt::ExtraButton13: return uint32_t(0x11f);
        // default should not occur; but if it does, then return Wayland's highest possible button number.
    default: return uint32_t(0x11f);
    }
}

/*!
 * \internal
 */
void QWaylandPointer::focusDestroyed(void *data)
{
    Q_D(QWaylandPointer);
    Q_UNUSED(data)
    d->focusDestroyListener.reset();

    d->seat->setMouseFocus(Q_NULLPTR);
    d->buttonCount = 0;
}

/*!
 * \internal
 */
void QWaylandPointer::pointerFocusChanged(QWaylandView *newFocus, QWaylandView *oldFocus)
{
    Q_UNUSED(newFocus);
    Q_D(QWaylandPointer);
    d->localPosition = QPointF();
    d->hasSentEnter = false;
    if (oldFocus) {
        uint32_t serial = d->compositor()->nextSerial();
        for (auto resource : d->resourceMap().values(oldFocus->surfaceResource()->client))
            d->send_leave(resource->handle, serial, oldFocus->surfaceResource());
        d->focusDestroyListener.reset();
    }
}

QT_END_NAMESPACE
