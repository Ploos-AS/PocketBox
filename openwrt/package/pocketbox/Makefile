include $(TOPDIR)/rules.mk

PKG_NAME:=pocketbox
PKG_VERSION:=0.1.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/pocketbox
  SECTION:=net
  CATEGORY:=Network
  TITLE:=PocketBox offline file portal and ASCII terminal
  DEPENDS:=+libc
endef

define Package/pocketbox/description
 Small offline HTTP file portal and ASCII terminal for supported MIPS OpenWrt routers.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/src
	$(CP) $(TOPDIR)/package/pocketbox/src/* $(PKG_BUILD_DIR)/src/
endef

TARGET_CFLAGS += -D_GNU_SOURCE -std=c11 -Os -Wall -Wextra

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -o $(PKG_BUILD_DIR)/pocketbox $(PKG_BUILD_DIR)/src/main.c
endef

define Package/pocketbox/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/init.d $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/pocketbox $(1)/usr/bin/pocketbox
	$(INSTALL_BIN) ./files/pocketbox.init $(1)/etc/init.d/pocketbox
	$(INSTALL_CONF) ./files/pocketbox.config $(1)/etc/config/pocketbox
endef

$(eval $(call BuildPackage,pocketbox))
