$(QML_RULES_DEPS): Makefile
	$(AM_V_at)$(MKDIR_P) $(@D)
	$(AM_V_GEN){ \
	  echo '$(@D)/qmldir.qrc: $(@D)/qmldir'; \
	  echo '$(@D)/res.qrc $(@D)/qmldir: $(@D)/qml_modules.stamp'; \
	  echo '$(@D)/qmlcachegen.rsp: $$(builddir)/$(@D)/res.qrc'; \
	  echo '$(@D)/qmlcache_loader.cpp: $(@D)/qmlcachegen.rsp'; \
	  echo '$(@D)/qmlassets.cpp: $$(builddir)/$(@D)/qmldir.qrc $$(builddir)/$(@D)/res.qrc'; \
	  echo '$(@D)/qmlassets.cpp: $$(libqml_module_$(@D)_a_QML) $$(libqml_module_$(@D)_a_Singleton_QML)'; \
	  echo 'qml_$(@D)_cachegen_sources = $$(libqml_module_$(@D)_a_QML) $$(libqml_module_$(@D)_a_Singleton_QML)'; \
	  echo 'qml_$(@D)_cachegen_qml = $$(qml_$(@D)_cachegen_sources:.qml=.cpp)'; \
	  echo 'qml_$(@D)_cachegen_js = $$(qml_$(@D)_cachegen_qml:.js=.cpp)'; \
	  echo '$$(qml_$(@D)_cachegen_js:.mjs=.cpp): $$(builddir)/$(@D)/res.qrc'; \
	} > $@.tmp && \
	if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv $@.tmp $@; fi
