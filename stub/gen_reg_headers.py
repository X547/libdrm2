import json

f = open('/boot/home/downloads/mesa-master/src/amd/registers/gfx6.json')
 
data = json.load(f)
 
for i in data['register_mappings']:
	print('case ' + hex(i['map']['at']) + ': return "' + i['name'] + '";')
