/*
 *   Copyright (C) 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQml 2.1
import org.kde.kirigami 2.6 as Kirigami

Kirigami.AboutPage
{
    actions.main: Kirigami.Action {
        function removeAmpersand(text) {
            return text.replace("&", "");
        }

        readonly property QtObject action: app.action("help_report_bug")
        text: removeAmpersand(action.text)
        enabled: action.enabled
        onTriggered: action.trigger()
        icon.name: app.iconName(action.icon)
    }

    aboutData: discoverAboutData
}
