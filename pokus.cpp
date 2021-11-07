#include <iostream>
#include <string>

using namespace std;

// main function to call above defined function.
int main ()
{
   string text;
   string * ptr;

   text = "ahoj!";
   ptr = &text;

   cout << text << endl;
   cout << *ptr << endl;

   text = "nazdar!";

   cout << text << endl;
   cout << *ptr << endl;

   ptr = new string("sbohem!");

   cout << text << endl;
   cout << *ptr << endl;

   delete ptr;

   return 0;

}