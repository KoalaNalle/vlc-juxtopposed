# Add after the configured test Makefile; these binaries are never installed.
ifndef NETWORK_OUT
$(error Set NETWORK_OUT to an owned native output directory)
endif
NETWORK_CPPFLAGS = $(DEFS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src
.PHONY: network-proxy-native
network-proxy-native: $(NETWORK_OUT)/test_network_proxy.exe
$(NETWORK_OUT):
	mkdir -p "$@"
$(NETWORK_OUT)/proxy_windows.o: $(srcdir)/src/network/proxy_windows.c $(top_srcdir)/src/win32/netconf.c | $(NETWORK_OUT)
	$(CC) $(NETWORK_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(NETWORK_OUT)/test_network_proxy.exe: $(NETWORK_OUT)/proxy_windows.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE) -lwinhttp -lnormaliz

.PHONY: network-settings-native
network-settings-native: $(NETWORK_OUT)/test_network_settings.exe
$(NETWORK_OUT)/settings_consumers.o: $(srcdir)/src/network/settings_consumers.c | $(NETWORK_OUT)
	$(CC) $(NETWORK_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(NETWORK_OUT)/test_network_settings.exe: $(NETWORK_OUT)/settings_consumers.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE)

.PHONY: network-proxy-policy-native
network-proxy-policy-native: $(NETWORK_OUT)/test_network_proxy_policy.exe
$(NETWORK_OUT)/proxy_policy.o: $(srcdir)/src/network/proxy_policy.c $(srcdir)/src/network/proxy_windows.c $(top_srcdir)/src/win32/netconf.c | $(NETWORK_OUT)
	$(CC) $(NETWORK_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(NETWORK_OUT)/test_network_proxy_policy.exe: $(NETWORK_OUT)/proxy_policy.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE) -lwinhttp -lnormaliz

.PHONY: network-pac-transport-native
network-pac-transport-native: $(NETWORK_OUT)/test_network_pac_transport.exe
$(NETWORK_OUT)/proxy_transport.o: $(srcdir)/src/network/proxy_transport.c $(srcdir)/src/network/proxy_windows.c $(top_srcdir)/src/win32/netconf.c | $(NETWORK_OUT)
	$(CC) $(NETWORK_CPPFLAGS) $(CFLAGS) -c $< -o $@
$(NETWORK_OUT)/test_network_pac_transport.exe: $(NETWORK_OUT)/proxy_transport.o
	$(LIBTOOL) --tag=CC --mode=link $(CC) $(CFLAGS) $(AM_LDFLAGS) -Wc,-static -o $@ $< $(LIBVLC) $(LIBVLCCORE) -lwinhttp -lnormaliz
