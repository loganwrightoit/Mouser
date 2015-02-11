#include <iostream>
using namespace std;

int gcd(int a, int b)
{
    return (b == 0) ? a / 2.0f : gcd(b, a%b);
}

int main()
{
    while (1)
    {
        int x;
        int y;
        cout << "Please input x y: ";
        cin >> x >> y;
        cout << endl;

        int result = gcd(x, y);
        cout << "GCD: " << result << " with " << ((x * y) / (result * result)) << " tiles." << endl << endl;
    }
}