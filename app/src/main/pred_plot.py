from pprint import pprint
adict = {}
list1 = []
list2 = []
with open('pred1.txt','r') as file:
	for line in file:
		a = int(line[:-1])
		list1.append(a)
with open('pred2.txt','r') as file:
	for line in file:
		a = int(line[:-1])
		list2.append(a)
		# if a not in adict:
		# 	adict[a] = 1
		# else:
		# 	adict[a] += 1
numRight = 0
for i,val in enumerate(list1):
	if val == list2[i]:
		numRight += 1
print(numRight, 600 - numRight)