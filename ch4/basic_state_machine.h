#include <iostream>

enum STATE { INIT = 0, INPUT, PRINT, EXIT };
STATE nState = INIT;
int nSum = 0;

void state_machine_start()
{
    while (nState != EXIT)
    {
        switch (nState)
        {
        case INIT:
            nSum = 0;
            nState = INPUT;
            break;
        case INPUT:
            {
                int nInput = 0;
                std::cin >> nInput;
                if (nInput == 0)
                    nState = PRINT;
                else if (nInput == -1)
                    nState = EXIT;
                else
                    nSum += nInput;
            }
            break;
        case PRINT:
            std::cout << nSum << std::endl;
            nState = INPUT;
            break;
        }
    }
}