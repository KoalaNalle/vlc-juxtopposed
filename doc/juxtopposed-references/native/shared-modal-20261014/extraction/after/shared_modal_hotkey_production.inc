// Exact production method/branch extraction; dependency adapters are in the test.
// SOURCE_SHA256 {"modules/gui/qt/maininterface/interface_window_handler.cpp": "d6e818ea0f26256462f56e2d834a7f846fbdae172c5fe86aa7ca1f0155a6b3e7", "modules/gui/qt/maininterface/mainctx.cpp": "b5a36669d051b71b01e445075d3e7c0cfbe243625e85e630795211f0c8db855e"}
static const char routingProductionIdentity[] = R"routing({"modules/gui/qt/maininterface/interface_window_handler.cpp": "d6e818ea0f26256462f56e2d834a7f846fbdae172c5fe86aa7ca1f0155a6b3e7", "modules/gui/qt/maininterface/mainctx.cpp": "b5a36669d051b71b01e445075d3e7c0cfbe243625e85e630795211f0c8db855e"})routing";
bool InterfaceWindowHandler::applyKeyEvent(QKeyEvent * event) const
{
    // A shared modal owns these keys, including mouse focus and cancellation
    // transitions. Returning false keeps press/release delivery in Qt controls.
    const auto *dialogs = Singleton<DialogsProvider>::getInstance<false>();
    if (dialogs && dialogs->qmlDialogVisible()) return false;

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
    // Direct QML and wheel dispatch must obey the same shared-modal boundary
    // as the native window event filter. Do not create a provider during setup.
    const auto *dialogs = Singleton<DialogsProvider>::getInstance<false>();
    if (dialogs && dialogs->qmlDialogVisible()) return;

    QKeyEvent event(QEvent::KeyPress, key, modifiers );
    int vlckey = qtEventToVLCKey(&event);
    var_SetInteger(vlc_object_instance(p_intf), "key-pressed", vlckey);
}
void MainCtx::sendVLCHotkey(int vlcHotkey)
{
    // Direct QML and wheel dispatch must obey the same shared-modal boundary
    // as the native window event filter. Do not create a provider during setup.
    const auto *dialogs = Singleton<DialogsProvider>::getInstance<false>();
    if (dialogs && dialogs->qmlDialogVisible()) return;

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
