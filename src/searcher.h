
#include <cstdint>
#include "stepper_motor.h"
#include "SongPlayer.h"
#include "music.h"

class PersonSearcher
{
    public:
        PersonSearcher();

        void findPerson(std::uint32_t ir);

    private:
        // We must store the left,right,up,down position to the turret.
        std::uint32_t verticalMotorPosition{5672}; // Goes from -70 0 70 (there are about 9 rotations needed which is then 9 * 4096 steps)
        std::uint32_t up_down_motor;
        StepperMotor verticalMotor{{21,20,19,18}};
        StepperMotor horizontalMotor{{14,15,16,17}};
        SongPlayer musicPlayer{0};



        //Now i  changed the speed to 
        // 52 teeth - 16 TEETh = 3.25 ratio. So now i only need 9 / 3.25 rotations to go back and forth from either end. Så 
        // 11343     steps to get to either end, we start then in 5672
        static constexpr std::uint32_t inputGearTeeth{52};
        static constexpr std::uint32_t outputGearTeeth{16};
        static constexpr std::uint32_t ratio{inputGearTeeth / outputGearTeeth};
        static constexpr std::uint32_t stepsNeededToGetToEitherEnd_{11343};
        static constexpr std::uint32_t irThreshhold_{30};

        bool goingRight_{true};
        
};

PersonSearcher::PersonSearcher()
{

}


void
PersonSearcher::findPerson(const std::uint32_t ir)
{
    if (ir > irThreshhold_)
    {
        printf("We found our target, ELIMINATE THEM\n");
        musicPlayer.playSong(melody, durations);
    }

    // We start at 18432 i.e pos 0,
    
    if (verticalMotorPosition < stepsNeededToGetToEitherEnd_ && goingRight_)
    {
        verticalMotor.stepForward();
        verticalMotorPosition++;
    }

    if (verticalMotorPosition > 0 && !goingRight_)
    {
        verticalMotor.stepBackward();
        verticalMotorPosition--;
    }

    if (verticalMotorPosition == stepsNeededToGetToEitherEnd_)
    {
        goingRight_ = false;
    }
    if (verticalMotorPosition == 0)
    {
        goingRight_ = true;
    }

    
    //Need about 4096 steps for one full rotation
    //From my very scientific test, i need abou 9 full rotations of the left thing to go from 70 to -70
    //Therefore we need to go back and forth 

    //Therefore i deduce that the positions in steps can be defined as such:
    // 0 = (9/2) * 4096 from -70 = 18432 steps. 

}
