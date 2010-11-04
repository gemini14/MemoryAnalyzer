#include "MemoryAnalyzer.h"

#ifdef _DEBUG
MemoryManager *mmgr = &MemoryManager::Get();
#endif

#include <iostream>
//#include <vld.h>
#include <vector>

class Complex
{
public:
	Complex(double a, double b) : r(a), c(b)
	{}
	void print() { std::cout << r << " " << c << std::endl; }

private:
	double r;
	double c;
};

using namespace std;

int main(int argc, char* argv[])
{
	mmgr->dumpLeaksToFile = true;

	int *i = new int[5];
	delete[] i;

	/*int *j = new int[5];
	j[0] = 4;
	cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;
	delete[] j;
	cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;*/

	/*char *c = new char[2];
	c[0] = 'h';
	c[1] = 'i';
	cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;
	delete[] c;
	cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;*/

	//Complex *classTest = new Complex(2.0, 3.0);
	//delete classTest;

	/*cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;
	
	cout << mmgr->GetCurrentMemory() << " " << mmgr->GetPeakMemory() << endl;*/
	
	vector<shared_ptr<Complex>> *testVec = new vector<shared_ptr<Complex>>;
	testVec->reserve(3);
	testVec->push_back(shared_ptr<Complex>(new Complex(5, 8)));
	testVec->push_back(shared_ptr<Complex>(new Complex(1, 499)));
	
	
	std::shared_ptr<Complex> test(new Complex(2, 4));

	delete testVec;
	return 0;
}