#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>

using namespace std;

// TODO
// or a vector to record taken directions.
struct Counters
{
	unsigned taken;
	unsigned notTaken;
};

struct FCounters
{
	// map<direction, count>
	map<unsigned, unsigned> directions;
};

/* translate string to int. */
unsigned int string_to_unsigned_int(string str)  
{
	//max value is 4294967296(2^32-1)
	unsigned int result(0);
    for (int i = str.size() - 1; i >= 0; i--)  
	{
		unsigned int temp(0),k = str.size() - i - 1;  
		if (isdigit(str[i])) 
	    {
			temp = str[i] - '0';  
			while (k--)  
				temp *= 10;  
			result += temp;  
        }else  
			break;  
	}
	return result;  
}

/* taken or not according curPC,nextPC,and ISAtype */
unsigned Taken(unsigned curPC, unsigned nextPC, string ISAtype)
{
	unsigned taken = 0;
	if (ISAtype == "1")
	{
		if((nextPC - curPC) > 4)
			taken = 1;
		else
			taken = 0;
	}else if (ISAtype == "0")
	{
		if((nextPC - curPC) > 2)
			taken = 1;
		else 
			taken = 0;
	}else
	{
		cout << "Wrong ISAtype @pc:" << curPC << endl;
	}
	return taken;
}

/* print cache */
/* n-way: n branches
 * sets: number of patterns
 * map<pc_index,map<hr,Counters>>
 */
void print(map<unsigned,map<unsigned,Counters> >& cache)
{
	// clear record.txt
	ofstream mClear("record.txt",ios::out);
	mClear.clear();
	mClear.close();

	ofstream write("record.txt",ios::app);
	map<unsigned, Counters>::iterator it;
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	map<unsigned, Counters> temp;

	int count_pc = 0;
	int count_per_entry = 0;
	int sum = 0;
	for (iter = cache.begin(); iter != cache.end(); iter++)
	{
		count_pc++;
		count_per_entry = 0;
		write << "-----" << iter->first << "--" << count_pc << "---" << endl;
		temp = (*iter).second;
		for (it = temp.begin(); it != temp.end(); it++)
		{
			count_per_entry++;
			// TODO
			write << (*it).second.taken << " " << (*it).second.notTaken << endl;
		}
		sum += count_per_entry;
	}
	cout << "total_entry: " << sum << endl;
	write.close();
}

/* overload operator+map<unsigned, Counters>
 * merge two maps: map<unsigned, Counters>
 * while merged, remove second map
 */
map<unsigned, Counters>& operator+(map<unsigned, Counters>& a, map<unsigned, Counters>& b)
{
	map<unsigned, Counters>::iterator ita;
	map<unsigned, Counters>::iterator itb;
	// merge the same part
	for (ita = a.begin();ita!=a.end();ita++)
	{
		itb = b.find(ita->first);
		if(itb != b.end())
		{
			// TODO
			// merge Counters
			(ita->second).taken += (itb->second).taken;
			(ita->second).notTaken += (itb->second).notTaken;

			b.erase(itb);
		}
	}
	// merge the different part
	for (itb = b.begin();itb!=b.end();)
	{
		a[itb->first] = itb->second;
		b.erase(itb++);
	}
	return a;
}

/* clean map<unsigned, map<unsigned, Counters> > cache
 * while cache[index] isEmpty, delete this entry.
 */
void mClean(map<unsigned, map<unsigned, Counters> >& cache)
{
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	for (iter = cache.begin(); iter != cache.end();)
	{
		if ((iter->second).empty())
			cache.erase(iter++);
		else
			iter++;
	}
	
}

/* @offset: bits of PC counted from LSB.
 * while offsets from different PCs are same, their map<unsigned, Counters> should be merged.
 * while call this function, please backup cache.
 */
void adjustWay(map<unsigned,map<unsigned,Counters> >&cache, int offset)
{
	map<unsigned, map<unsigned, Counters> > temp;

	unsigned collapsed_index = 0;
	
	// get mask
	unsigned mask = 0;
	for (int i = 0; i < offset; i++)
		mask |= (1 << i);

	// map<offseted pc, vector<origine pc> >
	map<unsigned, vector<unsigned> > collapse;
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	for (iter = cache.begin(); iter != cache.end(); iter++)
	{
		collapsed_index = iter->first & mask;
		collapse[collapsed_index].push_back(iter->first);
	}

	// merge same offsets
	map<unsigned, vector<unsigned> >::iterator it;
	vector<unsigned>::iterator vc;
	map<unsigned, Counters> sum;
	for (it = collapse.begin(); it != collapse.end(); it++)
	{
		for (vc = (it->second).begin(); vc != (it->second).end(); vc++)
		{
			sum = sum + cache[(*vc)];
		}
		temp[it->first] = sum;
		sum.clear();
	}

	// clean cache
	//mClean(cache);
	
	cache = temp;
	temp.clear();
}

/* merge PHTs.
 * n : count from MSB of PHT(20bits here)
 */
void adjustSet(map<unsigned, map<unsigned, Counters> >&cache, int n)
{
	unsigned mask = 0;
	for (int i = 0; i < (20 - n); i++)
	{
		mask |= (1 << i);
	}
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	map<unsigned, Counters>::iterator it;
	map<unsigned, Counters>::iterator newIT;
	map<unsigned, Counters> pht;
	map<unsigned, Counters> newPHT;
	unsigned hr;
	for (iter = cache.begin(); iter != cache.end(); iter++)
	{
		pht = iter->second;
		for (it = pht.begin(); it != pht.end(); it++)
		{
			hr = it->first & mask;
			newIT = newPHT.find(hr);
			if (newIT != newPHT.end()) {
				// TODO
				// merge Counters
				newPHT[hr].taken += (it->second).taken;
				newPHT[hr].notTaken += (it->second).notTaken;
			}else
			{
				newPHT[hr] = it->second;
			}
		}
		iter->second = newPHT;
		newPHT.clear();
	}
}

// TODO
// cal Counters' entropy
double entropy(Counters& ct)
{
	double total = ct.taken + ct.notTaken;
	double takenRate = (double)ct.taken / total;
	return 2 * min(takenRate, 1 - takenRate);
}

// TODO
/* num: taken + notTaken  (?) */
// cal total num of a Counters
unsigned total(Counters& ct)
{
	return ct.taken + ct.notTaken;
}

// cal total num of a PHT
unsigned totalN(map<unsigned, Counters>& pht)
{
	map<unsigned, Counters>::iterator it;
	unsigned sum = 0;
	for (it = pht.begin(); it != pht.end(); it++)
	{
		sum += total(it->second);
	}
	return sum;
}

// cal total num of a cache
unsigned calTotalN(map<unsigned, map<unsigned, Counters> >& cache)
{
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	map<unsigned, Counters>::iterator it;
	map<unsigned, Counters> pht;
	unsigned sum = 0;
	for (iter = cache.begin(); iter != cache.end(); iter++)
	{
		pht = iter->second;
		for (it = pht.begin(); it != pht.end(); it++)
		{
			sum += total(it->second);
		}
	}
	return sum;
}
double calEntropy(map<unsigned, map<unsigned, Counters> >& cache)
{
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	map<unsigned, Counters>::iterator it;
	map<unsigned, Counters> pht;
	double sum = 0;
	double totalNum = 0;
	double Entropy = 0;
	
	for (iter = cache.begin(); iter != cache.end(); iter++)
	{
		pht = iter->second;
		for (it = pht.begin(); it != pht.end(); it++)
		{
			totalNum += total(it->second);
			sum += total(it->second) * entropy(it->second);
		}
	}
	Entropy = sum / totalNum;
	return Entropy;
}

/* calculate global and local entropy per branch, 
 * take the minimum of the two, 
 * and average that minimum over all branches
 */
double calTournamentEntropy(map<unsigned, map<unsigned, Counters> >& gCache, map<unsigned, map<unsigned, Counters> >& lCache)
{
	map<unsigned, map<unsigned, Counters> >::iterator iter;
	map<unsigned, Counters>::iterator it;
	map<unsigned, Counters> pht;
	map<unsigned, Counters> lPht;
	double sum = 0;
	double MinE = 0;
	double MinT = 0;
	double Entropy = 0;

	double tempA;
	double tempB;
	double temp = 0;
	
	for (iter = gCache.begin(); iter != gCache.end(); iter++)
	{
		pht = iter->second;
		lPht = lCache[iter->first];
		for (it = pht.begin(); it != pht.end(); it++)
		{
			tempA = entropy(lPht[it->first]);
			tempB = entropy(it->second);
			MinE = tempA < tempB ? tempA : tempB;

			tempA = total(lPht[it->first]);
			tempB = total(it->second);
			MinT = tempA < tempB ? tempA : tempB;

			temp += MinT;
			sum += MinT * MinE;
		}
	}
	cout << "sum: " << sum << " temp: " << temp << endl;
	Entropy = sum / temp;
	return Entropy;
}

int main()
{
	ifstream read("branch_record.txt",ios::binary);
	if (!read.is_open())
	{
		cout << "Error Opening File." << endl;
		exit(1);
	}
	string line;
	istringstream stream;
	string curPC,nextPC,state,ISAtype;

	unsigned instShiftAmt = 2;

	unsigned cur_pc = 0;
	unsigned next_pc = 0;
	
	unsigned index_way = 0;
	unsigned local_index_way = 0;
	unsigned global_index_set = 0;
	unsigned local_index_set = 0;
	unsigned taken = 0;

	unsigned length = 20;
	unsigned mask = (1 << length) - 1;

	// global registor
	unsigned globalRegistor = 0;

	/* n-way set association cache:
	 * map<way, map<set,Counters> >
	 */
	map<unsigned, map<unsigned,Counters > > globalCache;

	// local registor
	map<unsigned, unsigned> localRegistor;
	unsigned localLength = 10;
	// local registor size, means how many entries for local PC.
	unsigned localRegistorSize = 1 << localLength;

	/* n-way set associtation cache:
	 * map<way, map<set,Counters> >
	 */
	map<unsigned, map<unsigned, Counters> > localCache;

	while (read.peek() != EOF)
	{
		getline(read,line);
		stream.str(line);
		if (stream.str()[0] == '-')
			continue;
		stream >> curPC;
		stream >> nextPC;
		stream >> state;
		stream >> ISAtype;

		cur_pc = string_to_unsigned_int(curPC);
		next_pc = string_to_unsigned_int(nextPC);

		taken = Taken(cur_pc,next_pc,ISAtype);

		// update cache
		index_way = cur_pc >> instShiftAmt;
		global_index_set = globalRegistor & mask;

		local_index_way = (cur_pc >> instShiftAmt) & (localRegistorSize - 1);
		local_index_set = localRegistor[local_index_way] & mask;
		if (taken == 1)
		{
			globalCache[index_way][global_index_set].taken++;
			localCache[index_way][local_index_set].taken++;
		}
		else
		{
			globalCache[index_way][global_index_set].notTaken++;
			localCache[index_way][local_index_set].notTaken++;
		}
		
		// update HR
		if(taken == 1)
		{
			globalRegistor = (globalRegistor << 1) | 1;
			localRegistor[local_index_way] = (localRegistor[local_index_way] << 1) | 1;
		}
		else
		{
			globalRegistor = globalRegistor << 1;
			localRegistor[local_index_way] = localRegistor[local_index_way] << 1;
		}
	}
	read.close();

//	adjustWay(globalCache,0);
//	adjustSet(globalCache,10);
	
	adjustWay(localCache,0);
	adjustSet(localCache,10);

//	print(globalCache);
	print(localCache);

//	cout << calEntropy(globalCache) << endl;
//	cout << calEntropy(localCache) << endl;
	cout << calTournamentEntropy(globalCache, localCache) << endl;

	return 0;
}
