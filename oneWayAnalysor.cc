#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <deque>

#define HR_MAX 20

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

/* print cache 
 * flag: cache type*/
/* n-way: n branches
 * sets: number of patterns
 * map<pc_index,map<hr,Counters>>
 */
void print(map<unsigned,Counters>& cache,string flag)
{
	string recordFile = "Record.txt";
	flag += recordFile;
	
	// clear record.txt
	ofstream mClear(flag,ios::out);
	mClear.clear();
	mClear.close();

	ofstream write(flag,ios::app);
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
 * n : count from MSB of HR_MAX(20bits here)
 * aliasing is an accuracy problem.
 */
void adjustSet(map<unsigned, Counters>&cache, int n)
{
	unsigned mask = 0;
	for (int i = 0; i < (HR_MAX - n); i++)
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
			// merge Counters: map<unsigned,unsigned>
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
	unsigned max;
	double total = 0;
	map<unsigned, unsigned>::iterator it;
	it = ct.directions.begin();
	mininal = it->second;
	max = it->second;
	for(it = ct.directions.begin(); it != ct.directions.end(); it++)
	{
		total += (double)it->second;
		if ((it->second) < mininal)
			mininal = it->second;
		if ((it->second) > max)
			max = it->second;
	}
	unsigned Size = ct.directions.size();

	// assume entropy is proportional to mininal probability.
	//return ((double)Size) * ((double)mininal) / total;

	// assume entropy is proportional to (1-max) probability.
	return ((double)Size) * ((double)1 - ((double)max / total));
}

// cal total num of a Counters: sum of all directions
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
 * take the minimum of the two(Action like Tournament BP), 
 * and average these minimums over all branches
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

		if(MinE == tempA)
			MinT = total(lCache[it->first]);
		else
			MinT = total(it->second);

		temp += MinT;
		sum += MinT * MinE;
	}
	Entropy = sum / temp;
	return Entropy;
}

int main(int argc, char* argv[])
{
	//unsigned interval = 1000000;
	unsigned interval = atoi(argv[1]) * 10000;
	ifstream read("branch_record.txt",ios::binary);
	if (!read.is_open())
	{
		cout << "Error Opening File." << endl;
		exit(1);
	}
	string line;
	istringstream stream;
	string curPC,nextPC,state,ISAtype;

	int total_count = 0;
	double interval_entropy = 0;
	string lTag = "local";
	string gTag = "global";

	unsigned instShiftAmt = 2;

	unsigned cur_pc = 0;
	unsigned next_pc = 0;
	
	unsigned index_way = 0;
	unsigned local_index_way = 0;
	unsigned global_index_set = 0;
	unsigned local_index_set = 0;
	unsigned taken = 0;

	unsigned length = HR_MAX;
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

		total_count++;

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
		
		// cal entropy of each interval length.
		if(total_count == interval)
		{
			adjustSet(globalCache,10);
			adjustSet(localCache,10);
			interval_entropy = calTournamentEntropy(globalCache, localCache);
			cout << interval_entropy << endl;
			ofstream write("entropy.txt",ios::app);
			write << interval_entropy << endl;
			write.close();
			total_count = 0;
			globalCache.clear();
			localCache.clear();
		}
	}
	read.close();

	if(total_count != 0)
	{
		adjustSet(globalCache,10);
		adjustSet(localCache,10);
	
//		print(globalCache,gTag);
//		print(localCache,lTag);
	
//		cout << calEntropy(globalCache) << endl;
//		cout << calEntropy(localCache) << endl;
		interval_entropy = calTournamentEntropy(globalCache, localCache);
		cout << interval_entropy << endl;
		ofstream write("entropy.txt",ios::app);
		write << interval_entropy << endl;
		write.close();
	}

	return 0;
}
