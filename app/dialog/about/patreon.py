import json
import requests
import os

url = 'https://www.patreon.com/api/oauth2/v2/campaigns/1478705/members?include=currently_entitled_tiers&fields%5Bmember%5D=full_name'
name_list = ''

while True:
    member_data = requests.get(url, headers = {"authorization": "Bearer " + os.environ.get('PATREON_KEY')})
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
text_file.write("#ifndef PATREON_H\n#define PATREON_H\n\n#include <QStringList>\n\nQStringList patrons = {\n%s\n};\n\n#endif // PATREON_H\n" % name_list)
text_file.close()
