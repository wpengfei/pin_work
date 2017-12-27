import scanf

trace_mem = {}

def print_mem_trace():
	for ad in trace_mem:
		print "=addr:", ad, "thread_num", trace_mem[ad]["thread_num"], "tid_list", trace_mem[ad]["tid_list"]
		for tid in trace_mem[ad]:
			if tid != "thread_num" and tid != "tid_list":
				print "==>tid", tid, "optag", trace_mem[ad][tid]["optag"]
				for it in trace_mem[ad][tid]["trace_list"]:
					print "====>", it 



file_lock_handler = open("file_lock.log", "r")
each_line = True
while each_line:
	each_line = file_lock_handler.readline()
	#print each_line

	if each_line:
		tid, op, time, callsite_v, entry_v = scanf.sscanf(each_line,"tid:%d,op:%c,time:%d,callsite_v:0x%x,entry_v:0x%x")

		#print tid, op, time,  hex(callsite_v), hex(entry_v)


'''
trace_mem = {"addr1": addr_dict1, "addr2":addr_dict2}

addr_dict = {"thread_num":2, "tid_list":tid_list, "tid1":tid_dict1, "tid2": tid_dict2}

tid_list = [tid1,tid2]

tid_dict = {"optag": 2, "trace_list": tlist}    # optag: 0,R; 1,W; 2,R&W

tlist = [trace1, trace2]



'''


file_mem_handler = open("file_mem_access.log", "r")
each_line = True
while each_line:
	each_line = file_mem_handler.readline()
	#print each_line

	if each_line:
		
		tid, op, time, ip, addr, rtn = scanf.sscanf(each_line,"tid:%d,op:%c,time:%d,ip:0x%x,addr:0x%x,rtn:%s\n")	
		
		ip = str(hex(ip))
		addr = str(hex(addr))
		tid = str(tid)
		trace = {}
		trace["tid"] = tid
		trace["op"] = op 
		trace["time"] = time
		trace["ip"] = ip
		trace["addr"] = addr
		trace["rtn"] = rtn

		#print trace["tid"], trace["op"], trace["time"],  trace["ip"], trace["addr"], trace["rtn"]

		
		if  trace_mem.has_key(addr):

			if trace_mem[addr].has_key(tid):

				optag = trace_mem[addr][tid]["optag"]

				if  op == "R" and optag == 1:
					optag = 2;
					trace_mem[addr][tid]["optag"] = optag
				elif op == "W" and optag == 0:
					optag = 2;
					trace_mem[addr][tid]["optag"] = optag

				
				trace_mem[addr][tid]["trace_list"].append(trace)

			else:
				tlist = []
				tlist.append(trace)

				tid_dict = {}
				if op == "R":
					tid_dict["optag"] = 0
				else:
					tid_dict["optag"] = 1
				tid_dict["trace_list"] = tlist


				trace_mem[addr]["thread_num"] = trace_mem[addr]["thread_num"] + 1
				trace_mem[addr]["tid_list"].append(tid)
				trace_mem[addr][tid] = tid_dict


		else:
			tlist = []
			tlist.append(trace)

			tid_dict = {}
			if op == "R":
				tid_dict["optag"] = 0
			else:
				tid_dict["optag"] = 1
			tid_dict["trace_list"] = tlist

			tid_list = [];
			tid_list.append(tid)

			addr_dict = {}
			addr_dict["thread_num"] = 1
			addr_dict["tid_list"] = tid_list
			addr_dict[tid] = tid_dict

			
			trace_mem[addr] = addr_dict

print_mem_trace()

print "----------------------------------------"

CI_list = []

def find_write_trace(tlist):
	ret = []
	for it in tlist:
		if it["op"] == "W":
			ret.append(it)
	return ret

def find_read_trace(tlist):
	ret = []
	for it in tlist:
		if it["op"] == "R":
			ret.append(it)
	return ret


def find_CI_from_thread_pair(tid_dict0, tid_dict1):
	if tid_dict0["optag"] == 0 and tid_dict1["optag"] == 0:
		return None
	elif tid_dict0["optag"] == 1 and tid_dict1["optag"] == 1:
		return None

	else: 
		tlist0 = tid_dict0["trace_list"]
		tlist1 = tid_dict1["trace_list"]

		l0 = len(tlist0)
		l1 = len(tlist1)

		if l0 >= 2:
			for i in range(l0):
				for j in range(i+1,l0):
					if tlist0[i]["op"] == "R" and tlist0[j]["op"] == "R":
						ret_list = find_write_trace(tlist1)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist0[i]
							ci["second"] = tlist0[j]
							ci["inter"] = it
							CI_list.append(ci)

					elif tlist0[i]["op"] == "W" and tlist0[j]["op"] == "R":
						ret_list = find_write_trace(tlist1)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist0[i]
							ci["second"] = tlist0[j]
							ci["inter"] = it
							CI_list.append(ci)
					elif tlist0[i]["op"] == "R" and tlist0[j]["op"] == "W":
						ret_list = find_write_trace(tlist1)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist0[i]
							ci["second"] = tlist0[j]
							ci["inter"] = it
							CI_list.append(ci)
					elif tlist0[i]["op"] == "W" and tlist0[j]["op"] == "W":
						ret_list = find_read_trace(tlist1)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist0[i]
							ci["second"] = tlist0[j]
							ci["inter"] = it
							CI_list.append(ci)

		if l1 >= 2:
			for i in range(l1): # switch tlist0 and tlist 1
				for j in range(i+1,l1):
					if tlist1[i]["op"] == "R" and tlist1[j]["op"] == "R":
						ret_list = find_write_trace(tlist0)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist1[i]
							ci["second"] = tlist1[j]
							ci["inter"] = it
							CI_list.append(ci)

					elif tlist1[i]["op"] == "W" and tlist1[j]["op"] == "R":
						ret_list = find_write_trace(tlist0)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist1[i]
							ci["second"] = tlist1[j]
							ci["inter"] = it
							CI_list.append(ci)
					elif tlist1[i]["op"] == "R" and tlist1[j]["op"] == "W":
						ret_list = find_write_trace(tlist0)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist1[i]
							ci["second"] = tlist1[j]
							ci["inter"] = it
							CI_list.append(ci)
					elif tlist1[i]["op"] == "W" and tlist1[j]["op"] == "W":
						ret_list = find_read_trace(tlist0)
						for it in ret_list:
							ci = {}
							ci["first"] = tlist1[i]
							ci["second"] = tlist1[j]
							ci["inter"] = it
							CI_list.append(ci)



def find_thread_paris_for_same_addr():

	for ad in trace_mem:
		if trace_mem[ad]["thread_num"] < 2:
			continue
		else:
			tid_list = trace_mem[ad]["tid_list"]
			l = len(tid_list)
			assert l >= 2
			if l == 2: # for most of the cases, only two threads involved
				tid_dict0 = trace_mem[ad][tid_list[0]]
				tid_dict1 = trace_mem[ad][tid_list[1]]

				find_CI_from_thread_pair(tid_dict0, tid_dict1)
						
			else:
				for i in range(l):
					tid_dict0 = trace_mem[ad][tid_list[i]]
					for j in range(i+1,l):
						tid_dict1 = trace_mem[ad][tid_list[j]]
						find_CI_from_thread_pair(tid_dict0, tid_dict1)

	



find_thread_paris_for_same_addr()


print "result =========================================="

for i in CI_list:
	print "first:",i["first"]
	print "second:",i["second"]
	print "inter:",i["inter"]
	print "-----------------------"







file_sync_handler = open("file_sync.log", "r")
each_line = True
while each_line:
	each_line = file_sync_handler.readline()
	#print each_line

	if each_line:
		tid, ty, time = scanf.sscanf(each_line,"tid:%d,type:%s,time:%d")

		print tid, ty, time


#dict_item = {}
					#dict_item['line'] = line
	