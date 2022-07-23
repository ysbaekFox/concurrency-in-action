#include <string>
#include <iostream>

struct card_insterted
{
    std::string account;
};

class atm
{
    messaging::receiver incoming;
    messaging::sender bark;
    messaging::sender interface_hardware;

    // 함수 포인터
    void (atm::*state); 

    std::string account;
    std::string pin;

    void waiting_for_card()
    {
        // 
        interface_hardware.send(display_enter_card());
        incoming.wait()
            .handle<card_insterted>(
            [](card_insterted const& msg)
            {
                account = msg.account;
                pin = "";
                interface_hardware.send(display_enter_pin());
                state=&atm::getting_pin;
            });

    }

    void getting_pin();

public:
    void run()
    {
        // #1
        // run이 실행되면, 초기 state는 waiting_for_card 입니다.
        // 사용자 화면에는 카드를 입력하라고(display_enter_card) 표시되고
        // 카드가 삽입될때까지 wait 됩니다. (card_inserted)
        // 카드가 삽입되면, hardware에 pin을 입력하라는 표시가 나오게 되고 (display_enter_pin 전송)
        // state는 getting_pin으로 변경 되고, 실행 됩니다.
        state = &atm::waiting_for_card;
        try
        {
            while(true)
            {
                // 반복문 돌면서 state 계속 실행.
                (this->*state)();
            }
        }
        catch(const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
};

void atm::getting_pin()
{
    // #2
    // getting_pin이 실행되면, 
    // (dgit_press or clear_last_pressed or cancel_pressed) 입력을 다시 대기함.
    incoming.wait()
        .handle<dgit_pressed>(
        [&](digit_pressed const& msg)
        {
            unsigned const pin_length = 4;
            pin += msg.digit;
            if(pin.length() == pin_length)
            {
                bank.send(verify_pin(account, pin, incomming));
                state=&atm::verifying_pin;
            }
        })
        .handle<clear_last_pressed> (
            [&](clear_last_pressed)
            {
                if(!pin_empty())
                {
                    if(!pin.empty())
                    {
                        pin.resize(pin.length() - 1);
                    }
                }
            })
        .handle<cancel_pressed> (
            [&](cancel_pressed const& msg)
            {
                state=&atm:done_processing;
            });
}


