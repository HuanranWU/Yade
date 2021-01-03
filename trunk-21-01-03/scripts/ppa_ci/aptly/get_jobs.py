#!/usr/bin/env python3

from urllib.request import Request, urlopen
import json

url = 'https://gitlab.com/api/v4/projects/10133144/jobs/?scope[]=success'
req = Request(url)
req.add_header('PRIVATE-TOKEN', 'XXXXX')
resp = urlopen(req)
content = urlopen(req).read()

j = json.loads(content.decode("utf-8"))
print(json.dumps(j, indent=2))
