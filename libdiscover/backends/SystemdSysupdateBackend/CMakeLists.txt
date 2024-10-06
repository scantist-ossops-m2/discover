file(GLOB systemdsysupdate-backend_SRCS
    "*.cpp"
    "*.h"
)

#set(atomupd1_xml com.steampowered.Atomupd1.xml)
#set_source_files_properties(${atomupd1_xml} PROPERTIES
#    INCLUDE "dbushelpers.h"
#    NA_NAMESPACE TRUE
#)
qt_add_dbus_interface(systemdsysupdate-backend_SRCS org.freedesktop.sysupdate1.xml sysupdate1)
#qt_add_dbus_interface(systemdsysupdate-backend_SRCS org.freedesktop.DBus.Properties.xml dbusproperties_interface)

ecm_qt_declare_logging_category(systemdsysupdate-backend_SRCS
    HEADER libdiscover_systemdsysupdate_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_systemdsysupdate_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend.systemdsysupdate
    DESCRIPTION "libdiscover (backend - systemdsysupdate)"
    EXPORT DISCOVER
)

kcoreaddons_add_plugin(systemdsysupdate-backend SOURCES ${systemdsysupdate-backend_SRCS} INSTALL_NAMESPACE "discover")

target_link_libraries(systemdsysupdate-backend
    Qt::Core
    Qt::Widgets
    Qt::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    Discover::Common
)
