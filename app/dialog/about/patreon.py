#  Oak Video Editor - Non-Linear Video Editor
#  Copyright (C) 2025 Olive CE Team
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
#

#  /***
#
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#  ***/
#

import json
import requests
import os

url = 'https://www.patreon.com/api/oauth2/v2/campaigns/1478705/members?include=currently_entitled_tiers&fields%5Bmember%5D=full_name'
name_list = ''

while True:
    member_data = requests.get(url, headers={"authorization": "Bearer " + os.environ.get('PATREON_KEY')})
    member_data_decoded = json.loads(member_data.text)

    for member in member_data_decoded["data"]:
        if len(member["relationships"]["currently_entitled_tiers"]["data"]) > 0:
            if member["relationships"]["currently_entitled_tiers"]["data"][0]["id"] == "3952333":
                if len(name_list) > 0:
                    name_list += ',\n'
                name = member["attributes"]["full_name"]
                name_list += "  QStringLiteral(\""
                name_list += name.translate(str.maketrans({
                    "\"": "\\\"",
                    "\\": "\\\\"
                }))
                name_list += "\")"

    if "links" in member_data_decoded:
        url = member_data_decoded["links"]["next"]
    else:
        break

text_file = open("patreon.h", "w", encoding="utf-8")
text_file.write(
    "#ifndef PATREON_H\n#define PATREON_H\n\n#include <QStringList>\n\nQStringList patrons = {\n%s\n};\n\n#endif // PATREON_H\n" % name_list)
text_file.close()
