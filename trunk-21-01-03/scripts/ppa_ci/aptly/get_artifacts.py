#!/usr/bin/env python3

import requests, zipfile, io

url = 'https://gitlab.com/api/v4/projects/10133144/jobs/artifacts/feature%2Fdailypackages/download?job=deb_bionic'

r = requests.get(url)
z = zipfile.ZipFile(io.BytesIO(r.content))
z.extractall()
