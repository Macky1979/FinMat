#include <iostream>
#include <string>
#include <vector>
#include <list>
#include "lib_date.h"

using namespace std;

int main() {
   const myDate &date(19791230);
   //cout << date.get_date_str() << endl;
   const int &x = 1;
   const int &y = 2;
   cout << x + y << endl;
   return 0;
}