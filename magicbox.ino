#include <Stepper.h>
#include <mutex>

int ledArray[] = {D4,D5,D6,D7};
int analogInArray[] = {A0,A1,A2,A3};
int digitalOutArray[] = {D0,D1,D2,D3};
int keyPressedLed = A4;
int steps = 100;
int stepperSpeed = 75;
int stepVal = 512;
Stepper stepper(steps,D4,D5,D6,D7);
Stepper rstepper(steps,D7,D4,D5,D6);

#define BUFFERSIZE 32
char keyMap [] = {'D','#','0','*','C','9','8','7','B','6','5','4','A','3','2','1'};
std::mutex passwordMutex;
char password [BUFFERSIZE + 1] = "12345AD";
char buffer [BUFFERSIZE + 1];
int bufferLength = 0;
char * iter = buffer;

int error = -1;
int ok = 0;

int setPassword(String newPassword)
{
    int code = ok;
    if(newPassword.length() > BUFFERSIZE)
    {
        Spark.publish("Set Password", "Error. New password \"" + newPassword + "\" is too big. Has a length of " + newPassword.length());
    }
    else
    {
        passwordMutex.lock();
        strcpy(password,newPassword.c_str());
        passwordMutex.unlock();
        Spark.publish("Set Password", "Success!! Set password value to \"" + newPassword + "\"!");
    }
    return code;
}

typedef struct KeyPressOutput
{
    int row;
    int col;
    int keyPressed;
};

void debug(String message, int value) 
{
    char msg [50];
    sprintf(msg, message.c_str(), value);
    Spark.publish("DEBUG", msg);
}

void debug(String message, int value1, int value2) 
{
    char msg [50];
    sprintf(msg, message.c_str(), value1, value2);
    Spark.publish("DEBUG", msg);
}

void setup() 
{
    for(int i = 0; i < 4; i++)
    {
        pinMode(ledArray[i],OUTPUT);
        pinMode(digitalOutArray[i],OUTPUT);
        pinMode(analogInArray[i],INPUT_PULLDOWN);
    }
    *iter = '\0';
    pinMode(keyPressedLed, OUTPUT);
    stepper.setSpeed(stepperSpeed);
    rstepper.setSpeed(stepperSpeed);
    Particle.function("setpassword", setPassword);
}

KeyPressOutput readKeyPad()
{
    KeyPressOutput retVal;
    retVal.row = 0;
    retVal.col = 0;
    retVal.keyPressed = 0;
    int buttonPressed = 0;
    int readButton = 0;
    int index = -1;
    for(int i = 0; i < 4; i++)
    {
        if(!buttonPressed)
        {
            digitalWrite(digitalOutArray[i],HIGH);
            for(int j = 0; j < 4; j++)
            {
                readButton = digitalRead(analogInArray[j]);
                delay(10);
                int secondRead = digitalRead(analogInArray[j]);
                if(secondRead)
                {
                    buttonPressed = 1;
                    retVal.row = i;
                    retVal.col = j;
                    retVal.keyPressed = 1;
                    break;
                }
            }
            digitalWrite(digitalOutArray[i],LOW);
        }
    }
    return retVal;
}

void clearLedArray()
{
    for(int i = 0; i < 4; i++)
    {
        digitalWrite(ledArray[i], LOW);
    }
}

void loop() 
{
    KeyPressOutput keyArgs = readKeyPad();
    if(keyArgs.keyPressed)
    {
        int index = (keyArgs.row << 2) + keyArgs.col;
        if(keyMap[index] == '*')
        {
            if(bufferLength == 0)
            {
                Spark.publish("Backspace", "Attempted to backspace and failed");
            }
            else
            {
                --bufferLength;
                sprintf(--iter,"\0");
                Spark.publish("Backspace","Ignore Last Line");
            }
        }
        else if(keyMap[index] == '#')
        {
            passwordMutex.lock();
            bufferLength = 0;
            Spark.publish("The Line Wound Up Being",buffer);
            int difference = strcmp(buffer,password);
            if(difference == 0)
            {
                Spark.publish("Password Check", "You have entered the correct password!");
                stepper.step(stepVal);
                rstepper.step(stepVal);
            }
            else
            {
                char errorMsg [] = "Expected \"%s\" and received \"%s\". Difference of %d";
                char errorStr [256];
                sprintf(errorStr,errorMsg,password,buffer,difference);
                Spark.publish("Password Check", errorStr);
            }
            iter = buffer;
            *iter = '\0';
            passwordMutex.unlock();
        }
        else if(bufferLength == BUFFERSIZE)
        {
            Spark.publish("You Entered", "ERROR: AT END OF BUFFER");
        }
        else
        {
            ++bufferLength;
            snprintf(iter++,2 * sizeof(char),"%c\0",keyMap[index]);
            char debugVal [] = { keyMap[index], '\0'};
            Spark.publish("You Entered",debugVal);
        }
        digitalWrite(keyPressedLed,HIGH);
        delay(100);
        digitalWrite(keyPressedLed,LOW);
    }
}