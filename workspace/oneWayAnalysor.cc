#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>

using namespace std;

struct Counters
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
void print(map<unsigned,Counters>& cache)
{
	// clear record.txt
	ofstream mClear("record.txt",ios::out);
	mClear.clear();
	mClear.close();

	ofstream write("record.txt",ios::app);
	map<unsigned, Counters>::iterator it;
	map<unsigned, unsigned>::iterator iter;

	int count = 0;
	for (it = cache.begin(); it != cache.end(); it++)
	{
		count++;
		write << "------" << endl;
		for (iter = (it->second).directions.begin(); iter != (it->second).directions.end(); iter++)
		{
			write << iter->first << " " << iter->second << endl;
		}
	}
	cout << "total_entry: " << count << endl;
	write.close();
}

/* overload operator+=map<unsigned, unsigned>
 * merge two maps: map<unsigned, unsigned>
 * while merged, remove second map
 */
map<unsigned, unsigned>& operator+=(map<unsigned, unsigned>& a, map<unsigned, unsigned>& b)
{
	map<unsigned, unsigned>::iterator ita;
	map<unsigned, unsigned>::iterator itb;
	// merge the same part
	for (ita = a.begin(); ita != a.end(); ita++)
	{
		itb = b.find(ita->first);
		if(itb != b.end())
		{
			// merge count
			ita->second += itb->second;
			ita->second += itb->second;

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

/* merge PHTs.
 * n : count from MSB of PHT(20bits here)
 * aliasing is an accuracy problem.
 */
void adjustSet(map<unsigned, Counters>&cache, int n)
{
	unsigned mask = 0;
	for (int i = 0; i < (20 - n); i++)
	{
		mask |= (1 << i);
	}
	map<unsigned, Counters>::iterator it;
	map<unsigned, Counters>::iterator newIT;
	map<unsigned, Counters> newPHT;
	unsigned hr;
	for (it = cache.begin(); it != cache.end(); it++)
	{
		hr = it->first & mask;
		newIT = newPHT.find(hr);
		if (newIT != newPHT.end()) {
			// merge Counters
			newPHT[hr].directions += (it->second).directions;
		}else
		{
			newPHT[hr] = it->second;
		}
	}
	cache = newPHT;
	newPHT.clear();
}

// cal Counters' entropy
double entropy(Counters& ct)
{
	unsigned mininal;
	double total = 0;
	map<unsigned, unsigned>::iterator it;
	it = ct.directions.begin();
	mininal = it->second;
	for(it = ct.directions.begin(); it != ct.directions.end(); it++)
	{
		total += (double)it->second;
		if ((it->second) < mininal)
			mininal = it->second;
	}
	unsigned Size = ct.directions.size();
	return ((double)Size) * ((double)mininal) / total;
}

/* num: taken + notTaken  (?) */
// cal total num of a Counters
unsigned total(Counters& ct)
{
	unsigned total = 0;
	map<unsigned, unsigned>::iterator it;
	for(it = ct.directions.begin(); it != ct.directions.end(); it++)
	{
		total += it->second;
	}
	return total;
}

double calEntropy(map<unsigned, Counters>& cache)
{
	map<unsigned, Counters>::iterator it;
	double sum = 0;
	double totalNum = 0;
	double Entropy = 0;
	
	for (it = cache.begin(); it != cache.end(); it++)
	{
		totalNum += total(it->second);
		sum += total(it->second) * entropy(it->second);
	}
	Entropy = sum / totalNum;
	return Entropy;
}

/* calculate global and local entropy per branch, 
 * take the minimum of the two, 
 * and average that minimum over all branches
 */
double calTournamentEntropy(map<unsigned, Counters>& gCache, map<unsigned, Counters>& lCache)
{
	map<unsigned, Counters>::iterator it;
	double sum = 0;
	double MinE = 0;
	double MinT = 0;
	double Entropy = 0;

	double tempA;
	double tempB;
	double temp = 0;
	
	for (it = gCache.begin(); it != gCache.end(); it++)
	{
		tempA = entropy(lCache[it->first]);
		tempB = entropy(it->second);
		MinE = tempA < tempB ? tempA : tempB;

		tempA = total(lCache[it->first]);
		tempB = total(it->second);
		MinT = tempA < tempB ? tempA : tempB;

		temp += MinT;
		sum += MinT * MinE;
	}
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

	/* 1-way set association cache:
	 * map<way, map<set,Counters> >
	 */
	map<unsigned,Counters> globalCache;

	// local registor
	map<unsigned, unsigned> localRegistor;
	unsigned localLength = 10;
	// local registor size, means how many entries for local PC.
	unsigned localRegistorSize = 1 << localLength;

	/* 1-way set associtation cache:
	 * map<way, map<set,Counters> >
	 */
	map<unsigned, Counters> localCache;

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

		globalCache[global_index_set].directions[next_pc]++;
		localCache[local_index_set].directions[next_pc]++;
		
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

//	adjustSet(globalCache,10);
	
	adjustSet(localCache,10);

//	print(globalCache);
	print(localCache);

//	cout << calEntropy(globalCache) << endl;
//	cout << calEntropy(localCache) << endl;
	cout << calTournamentEntropy(globalCache, localCache) << endl;

	return 0;
}
