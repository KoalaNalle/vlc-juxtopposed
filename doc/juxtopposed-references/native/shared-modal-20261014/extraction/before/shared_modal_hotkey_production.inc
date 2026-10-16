// Exact production method/branch extraction; dependency adapters are in the test.
// SOURCE_SHA256 {"modules/gui/qt/maininterface/interface_window_handler.cpp": "a0153ccfbb49d1f5e9fb8b81bea00b21a5e63c5424542d0dd74823c53357cae5", "modules/gui/qt/maininterface/mainctx.cpp": "025bcf34108679e33865136a19df44ea34f897e24f1a8174dce281ab9c4ec101"}
static const char routingProductionIdentity[] = R"routing({"modules/gui/qt/maininterface/interface_window_handler.cpp": "a0153ccfbb49d1f5e9fb8b81bea00b21a5e63c5424542d0dd74823c53357cae5", "modules/gui/qt/maininterface/mainctx.cpp": "025bcf34108679e33865136a19df44ea34f897e24f1a8174dce281ab9c4ec101"})routing";
bool InterfaceWindowHandler::applyKeyEvent(QKeyEvent * event) const
{
    int key = event->key();

    // NOTE: We have to make sure tab and backtab are never used as hotkeys. When browsing the
    //       MediaLibrary, we let the view handle the navigation keys.
    if (key == Qt::Key_Tab || key == Qt::Key_Backtab || m_mainCtx->preferHotkeys() == false)
    {
        return false;
    }

    QQuickItem * item = p_intf->p_compositor->activeFocusItem();

    // NOTE: When the item has visual focus we let it handle the key. When the item does not
    //       inherit from QQuickControl we have to declare the 'visualFocus' property ourselves.
    if (item)
    {

        QVariant visualFocus = QQmlProperty::read(item, "visualFocus", qmlContext(item));
        if (visualFocus.isValid())
        {
            if (visualFocus.toBool())
                return false;
        }
        //while being QuickControls TextField and TextArea don't provide visualFocus property
        //here we check their (non control) parent class, this should cover all text input widgets
        else if (item->inherits("QQuickTextInput") || item->inherits("QQuickTextEdit"))
        {
            QVariant activeFocus = QQmlProperty::read(item, "activeFocus", qmlContext(item));
            if (activeFocus.isValid())
            {
                if (activeFocus.toBool())
                    return false;
            }
        }
    }

    event->accept();

    return true;
}
void MainCtx::sendHotkey(Qt::Key key , Qt::KeyboardModifiers modifiers)
{
    QKeyEvent event(QEvent::KeyPress, key, modifiers );
    int vlckey = qtEventToVLCKey(&event);
    var_SetInteger(vlc_object_instance(p_intf), "key-pressed", vlckey);
}
void MainCtx::sendVLCHotkey(int vlcHotkey)
{
    var_SetInteger(vlc_object_instance(p_intf), "key-pressed", vlcHotkey);
}
bool InterfaceWindowHandler::eventFilter(QObject *, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::KeyPress:
    {
        QKeyEvent * keyEvent = static_cast<QKeyEvent *> (event);

        /* easter eggs sequence handling */
        if ( keyEvent->key() == kc[ i_kc_offset ] )
            i_kc_offset++;
        else
            i_kc_offset = 0;

        if ( i_kc_offset == ARRAY_SIZE( kc ) )
        {
            i_kc_offset = 0;
            emit kc_pressed();
        }

        if (applyKeyEvent(keyEvent) == false)
            return false;

        m_mainCtx->sendHotkey(static_cast<Qt::Key> (keyEvent->key()), keyEvent->modifiers());

        return true;
    }
    case QEvent::KeyRelease:
    {
        return applyKeyEvent(static_cast<QKeyEvent *> (event));
    }
    default: return false;
    }
}
