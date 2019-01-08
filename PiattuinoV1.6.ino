/*
  Project: Arduino MIDI contoled plate fone
  named Piattuino
  
  11.9.2018
  Heinz Pieren
  
  With 30 plates I made a percussion-instrument, witch can be played like a xylofone with wooden sticks.
  Because of training lacks of myself I had the idea to robot the instrument with solenoids and control the
  hole thing trough a MIDI interface.

  The program has the following parts:
  - Solenoid driver for the 30 plates in the instrument
  - LED Display for Each Solenoid
  - MIDI RX and TX Driver and Interpreter for a given MIDI-Channel
  - Setup and Test modules  
  
  V1.1
  - Possibility of two channels to be listened and executed by the piattuino
  
  V1.2
  - New MIDI-Latency function implemented available in Setup
  
  V1.3
  - Channel Input changed from 90..9F to 1..16
  - Setup 0 + 1 changed
  - Last values (channels,transpose,delay) in Nonvolatile memory
  
  V1.4
  - Quick Setup for channel selection Song 1..10
  R1: 11th setup nr implemented
  
  V1.5
  - Display Supply Voltage on Start and when Showing Channel and transpose values
  R1
  - SetUp Channel Roll Over 1->16 and 16->1
  
  V1.6
  - Send all notes off in case of startup and buffer overflow
  
*/

#define Version "V1.6 "
#define Release "R0"
#define SegDispVersion "A160"

//-----------------------------------------------------------------------
// EEPROM Library

#include <EEPROM.h>

//-----------------------------------------------------------------------
// All Notes Off Command String
// Midi Channel Mode Message 0xB0..0xBF 176--191
// 0x40 64 Controller Change Damper Pedal on/off (Sustain)
// 0x42 66 Controller Change Sostenuto On/Off
// 0x45 69 Controller Change Hold 2
// 0x78 120 All Sound Off
// 0x79 121 Reset All Controllers 
// 0x7B 123 All Notes Off Turn off all notes which for which a note-on MIDI message has been received. 

// Sent when a change is made in a pitch-bender lever. 
// 0xE0..EF 224..239 
// 0		-> Center
// 0x40 64  -> Center

#define ANOLen 192
const byte AllNotesOff [ANOLen] = 			//by Windows Miditester
{176,64,0,177,64,0,178,64,0,179,64,0,180,64,0,181,64,0,182,64,0,183,64,0,184,64,0,185,64,0,186,64,0,187,64,0,188,64,0,189,64,0,190,64,0,191,64,0,
176,66,0,177,66,0,178,66,0,179,66,0,180,66,0,181,66,0,182,66,0,183,66,0,184,66,0,185,66,0,186,66,0,187,66,0,188,66,0,189,66,0,190,66,0,191,66,0,
176,69,0,177,69,0,178,69,0,179,69,0,180,69,0,181,69,0,182,69,0,183,69,0,184,69,0,185,69,0,186,69,0,187,69,0,188,69,0,189,69,0,190,69,0,191,69,0,
176,123,0,177,123,0,178,123,0,179,123,0,180,123,0,181,123,0,182,123,0,183,123,0,184,123,0,185,123,0,186,123,0,187,123,0,188,123,0,189,123,0,190,123,0,191,123,0,};

#define VPLen 192
#define AllSoundOffMsg "ASOF"

const byte VolcanoPanic [VPLen] =			// by Android volcano App
{176,120,0,176,121,0,176,123,0,224,0,64,
177,120,0,177,121,0,177,123,0,225,0,64,
178,120,0,178,121,0,178,123,0,226,0,64,
179,120,0,179,121,0,179,123,0,227,0,64,
180,120,0,180,121,0,180,123,0,228,0,64,
181,120,0,181,121,0,181,123,0,229,0,64,
182,120,0,182,121,0,182,123,0,230,0,64,
183,120,0,183,121,0,183,123,0,231,0,64,
184,120,0,184,121,0,184,123,0,232,0,64,
185,120,0,185,121,0,185,123,0,233,0,64,
186,120,0,186,121,0,186,123,0,234,0,64,
187,120,0,187,121,0,187,123,0,235,0,64,
188,120,0,188,121,0,188,123,0,236,0,64,
189,120,0,189,121,0,189,123,0,237,0,64,
190,120,0,190,121,0,190,123,0,238,0,64,
191,120,0,191,121,0,191,123,0,239,0,64};



/* Supply Voltage on Pin  */
#define batteryVoltageInput 0   // Analog Input A0

// Supplyvoltage 

float SupplyVoltage = 0;
int SupplyVoltageZehntel=0;

//Solenoid stuff
unsigned int TimeNow;

#define defStrokeWait 1000
#define defStrokeOn 11

// MIDI Song Setup Stuff
byte SongSetUpNr;					//thera are SongSetUpNrmax Songs Setup possibilities

#define defSongSetUpNr 1
#define SongSetUpNrmin 1
#define SongSetUpNrmax 11

// MIDI default stuff

#define defdelayMIDI 0
#define delayMIDImin 0
#define delayMIDImax 975

#define defMIDINoteOn1 0x90
#define defMIDINoteOn2 0x90
#define MIDINoteONmin  0x90
#define MIDINoteONmax  0x9F

#define defMIDITranspose1 0
#define defMIDITranspose2 0

#define MIDITransposemin -36
#define MIDITransposemax  36


//unsigned int StrokeWait=defStrokeWait;
unsigned int StrokeOn=defStrokeOn;  	//for JF-1039B Solenoid now individual to plates

//plates

const byte c5=0;		//Tone c5 is plate nr. 0
const byte c5s=1;
const byte d5=2;
const byte d5s=3;
const byte e5=4;
const byte f5=5;
const byte f5s=6;
const byte g5=7;
const byte g5s=8;
const byte a5=9;
const byte a5s=10;
const byte b5=11;

const byte c6=12;		//Tone c5 is plate nr. 0
const byte c6s=13;
const byte d6=14;
const byte d6s=15;
const byte e6=16;
const byte f6=17;
const byte f6s=18;
const byte g6=19;
const byte g6s=20;
const byte a6=21;
const byte a6s=22;
const byte b6=23;

const byte c7=24;		//Tone c7 is plate nr. 0
const byte c7s=25;
const byte d7=26;
const byte d7s=27;
const byte e7=28;
const byte f7=29;		//the last plate is f7 nr 29

//MIDI Latency/Delay stuff

unsigned int delayMIDI;    //Delay of StrokeOn in ms
bool delayMIDIOn=true;			// the flag that shows that the delay function is on

const unsigned int delayBufSiz=100;		// number of notes stored in delay buffer

unsigned int 	delayStartTim[delayBufSiz];		//All start times of the Plate Notes
byte 			delayedPlate[delayBufSiz];		//the plate number to be delayed

unsigned int delBufInPointer=0;					// the pointer for the input data of the ringbuffer
unsigned int delBufOutPointer=0;					// the pointer for the output data of the ringbuffer



//MIDI Analyze mode Data

#define AllMidiChannels 16			//there are 16 channels to analyze

byte NoteOnChannel[AllMidiChannels];	//here we have NoteOn for Every channel
byte lowestNote[AllMidiChannels];	//the lowest tone for each channel
byte highestNote[AllMidiChannels];  //the higest note


#define AllPlates 	30				//we have 30 plates

//									0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29
// const unsigned int StrokeTime[] = {10,10,10,11,11,10,11,11,11,10,10,10, 8, 9,11,11, 9,10,10, 9, 9,10,10,11, 9,11, 9, 9,11,11};
//						   		   c5 c5#d5 d5#e5 f5 f5#g5 g5#a5 a5#b5 c6 c6#d6 d6#e6 f6 f6#g6 g6#a6 a6#b6 c7 c7#d7 d7#e7 f7 

//byte 			ToneState[AllPlates]; 			//MIDI tone state0= OFF, 50H=ON
unsigned int 	StrokeStart[AllPlates];  		//the lenght of the stroke of the soleneoid
bool 			SolenoidON[AllPlates];			// true means the solenoid is on

const byte SolenoidPortBase=22;		//all solenoids are conected from port 22 upwards

//MIDI Stuff
const byte lowestNoteOn = 24;   // Means C2
const byte highestNoteOn = 127;   // Means G8
const byte lowestPlateNote = 60;		//the bigest plate is tone C5
const byte highestPlateNote = 89;	//the smallest plate is tone E7
const byte lowestStroke = 1;	//minimum stroke value
const byte highestStroke = 127;	//maximum stroke value

byte MIDIstate=0;
byte MIDINote;

byte MIDINoteOn1;			//Command Note ON Channel ..
int MIDITranspose1;			//transpose value

byte MIDINoteOn2;			//Command Note ON Channel ..
int MIDITranspose2;			//transpose value

bool MIDINoteValid=false;
int PlateNum;					//Plate 0 is C5, Plate 29 ist F7

//Display Stuff
char sevenSegmentLEDLine[10]; 	//Used for sprintf to show text on display	

#define dispTime 1000			//one secont of display time

int SetUpstate=0;				//regular operation

//plate test stuff

int testPlate;
int testStrokeOn;
int testStrokeOff;

unsigned int testStrokeEnd;
	
// -----------------------------------------------------------------------------
void GetMIDITone()
{
	
byte recByte;

if (Serial1.available())
//while (Serial1.available()) //cans also be used but at the moment no advantage
	{
		if (Serial1.available()>62) 
		{ 
			secureAllSolenoidsOff(); // all ouputs to 0
			Serial.println("Overflow"); 
			sprintf(sevenSegmentLEDLine, "E%3d", Serial1.available()); //Error with bytes available
			displayLine();
			Serial1.flush();
			SendAllNotesOFFToMIDIout();
			while(1);  //Stop processing data
		} 
		
		//Serial.println(Serial1.available());
		recByte=Serial1.read();		//read MIDI Data
		Serial1.write(recByte);		//echo to MIDI OUT
		
		if (!(recByte==254))
		{
				
			//if(recByte>0x8F && recByte<0xA0)Serial.println(recByte);
			//Serial.println(recByte);
		
			switch (MIDIstate) 
			{
			case 0: 
				//Serial.println(MIDIstate);
				if ((recByte==MIDINoteOn1)) MIDIstate=1;		//means note on received
				if ((recByte==MIDINoteOn2)) MIDIstate=11;		//means note on received
			break;
			case 1:				//Note On 1
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn)) 
				{
					MIDIstate=2;
					MIDINote=recByte;
				}
				else MIDIstate=0;
			break;
			case 2:
				//Serial.println(MIDIstate);
				if (recByte==0) MIDIstate=3;
				else if ((recByte>=lowestStroke)&&(recByte<=highestStroke))
				{
					MIDINoteValid=true;
					PlateNum=MIDINote-lowestPlateNote+MIDITranspose1;
					MIDIstate=3;
				}
			break;
			case 3:
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn))  
				{
					MIDIstate=2;
					MIDINote=recByte;
				}
				else if ((recByte==MIDINoteOn1)) MIDIstate=1;		//means note on received
				else if ((recByte==MIDINoteOn2)) MIDIstate=11;		//means note on received
				else MIDIstate=0;
				break;
			case 11:							//Note On 2
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn)) 
				{
					MIDIstate=12;
					MIDINote=recByte;
				}
				else MIDIstate=0;
			break;
			case 12:
				//Serial.println(MIDIstate);
				if (recByte==0) MIDIstate=13;
				else if ((recByte>=lowestStroke)&&(recByte<=highestStroke))
				{
					MIDINoteValid=true;
					PlateNum=MIDINote-lowestPlateNote+MIDITranspose2;
					MIDIstate=13;
				}
			break;
			case 13:
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn))  
				{
					MIDIstate=12;
					MIDINote=recByte;
				}
				else if ((recByte==MIDINoteOn1)) MIDIstate=1;		//means note on received
				else if ((recByte==MIDINoteOn2)) MIDIstate=11;		//means note on received
				else MIDIstate=0;
				break;
			//default:
			//break;
			}
		} //end of not recByte==255
	} //end of serial.available()	
		
if 	(MIDINoteValid) 
	{
	if (delayMIDIOn) SetPlateToneOnDelayed(PlateNum);
	else SetPlateToneOn(PlateNum);
	MIDINoteValid=false;
	}
}


// -----------------------------------------------------------------------------
unsigned int millisInt()		//gets the time in unsigned integer
{

unsigned long millisTime;
unsigned int millisIntTime;
	
	millisTime=millis();
    millisIntTime=word(millisTime);
	return millisIntTime;
	
	
}

// -----------------------------------------------------------------------------
void ControlAllPlatesToneOff()
{

TimeNow=millisInt();
	
	for (int i=0; i<AllPlates; i++)
	{
		if (((TimeNow-StrokeStart[i])> StrokeOn) && SolenoidON[i])
		//if (((TimeNow-StrokeStart[i])> StrokeTime[i]) && SolenoidON[i])
		{
			SolenoidON[i]=false;
			updateSolenoid(i);
			
		}
		
	}
	
}

// -----------------------------------------------------------------------------
void ControlDelayedPlatesToneOn()
{

TimeNow=millisInt();
bool doCheck;

if (delBufInPointer != delBufOutPointer) doCheck=true;
else doCheck=false;
	
	while (doCheck)
	{
		if (((TimeNow-delayStartTim[delBufOutPointer])> delayMIDI))
		//if (((TimeNow-StrokeStart[i])> StrokeTime[i]) && SolenoidON[i])
		{
			SetPlateToneOn(delayedPlate[delBufOutPointer]);
			delBufOutPointer=(delBufOutPointer+1)%delayBufSiz;
			if (delBufInPointer == delBufOutPointer) doCheck=false;
		}
		else doCheck=false;
		
	}
	
}

// -----------------------------------------------------------------------------
void ControlAllSwitches()
{
	byte SNr;

if (digitalRead(A11)==LOW)		// enter Switch control
{

if (digitalRead(A8)==LOW) SetUpstate=200;		//Make Quick Song 1..10 Setup	
if (digitalRead(A9)==LOW) SetUpstate=300;		// send all sound off to MIDI Out
if (digitalRead(A8)==LOW && digitalRead(A9)==LOW) SetUpstate=1;	//T1 + T2 Enter Setup Mode
if (digitalRead(A10)==LOW) SetUpstate=100;		//T3 Show Values if long pressed Play Test Song

	
while (SetUpstate!=0)
{
	secureAllSolenoidsOff();
	switch (SetUpstate) 
	{
    case 1:		//Display all Setup Panels for selection
		SNr=0;
		do
		{
			sprintf(sevenSegmentLEDLine, "SEt%1d",SNr); //test plate mode
			displayLine();
			delay(dispTime);
			
			if (digitalRead(A11)==LOW) SNr=(SNr+1)%6;
			if (digitalRead(A12)==LOW) {if (SNr==0) SNr=5; else SNr--;}
			
			if (digitalRead(A8)==LOW) SetUpstate=10+SNr;		//Ok Go to SNr 0..5
			if (digitalRead(A9)==LOW) SetUpstate=0;		//quit
	
		} while (SetUpstate==1);
	break;
	
	case 10:		//Change NoteON1 and transpose1
		sprintf(sevenSegmentLEDLine, "1c%2d", (MIDINoteOn1-0x8F)); //Show NoteOn1 Channel 0x90 -> 1 
		displayLine();
		delay(dispTime);
		//sprintf(sevenSegmentLEDLine, "1%3d", MIDITranspose1); //Show t -> transpose
		//displayLine();
		//delay(dispTime);
		do
		{
			while (digitalRead(A8)==LOW)
			{
			if (digitalRead(A8)==LOW && digitalRead(A11)==LOW) ++MIDINoteOn1; //change MIDI Channel up
			if (digitalRead(A8)==LOW && digitalRead(A12)==LOW) --MIDINoteOn1; //change MIDI Channel up
			if (MIDINoteOn1>MIDINoteONmax) MIDINoteOn1=MIDINoteONmin;	//roll over to minimum
			if (MIDINoteOn1<MIDINoteONmin) MIDINoteOn1=MIDINoteONmax;  // roll over zo maximum
			// MIDINoteOn1=constrain(MIDINoteOn1,MIDINoteONmin,MIDINoteONmax);
			sprintf(sevenSegmentLEDLine, "1c%2d", (MIDINoteOn1-0x8F)); //Show NoteOn1 Channel
			displayLine();
			delay(dispTime);
			}
			
			while (digitalRead(A9)==LOW)
			{
			if (digitalRead(A9)==LOW && digitalRead(A11)==LOW) MIDITranspose1=MIDITranspose1+12; //transpose up
			if (digitalRead(A9)==LOW && digitalRead(A12)==LOW) MIDITranspose1=MIDITranspose1-12; //transpose down
			MIDITranspose1=constrain(MIDITranspose1,MIDITransposemin,MIDITransposemax);
			sprintf(sevenSegmentLEDLine, "1%3d", MIDITranspose1); //Show t -> transpose
			displayLine();
			delay(dispTime);
			}
			
			if (digitalRead(A10)==LOW)  // quit Button pressed
			{
				//MIDINoteOn2=MIDINoteOn1;
				//MIDITranspose2=MIDITranspose1;
				SetUpstate=11;
			}
			
		} while (SetUpstate==10);
	break;
	
	case 11:		//Change NoteON2 and transpose2
		sprintf(sevenSegmentLEDLine, "2c%2d", (MIDINoteOn2-0x8F)); //Show NoteOn2 Channel
		displayLine();
		delay(dispTime);
		//sprintf(sevenSegmentLEDLine, "2%3d", MIDITranspose2); //Show 2ntranspose
		//displayLine();
		//delay(dispTime);
		do
		{
			while (digitalRead(A8)==LOW)
			{
			if (digitalRead(A8)==LOW && digitalRead(A11)==LOW) ++MIDINoteOn2; //change MIDI Channel up
			if (digitalRead(A8)==LOW && digitalRead(A12)==LOW) --MIDINoteOn2; //change MIDI Channel up
			if (MIDINoteOn2>MIDINoteONmax) MIDINoteOn2=MIDINoteONmin;	//roll over to minimum
			if (MIDINoteOn2<MIDINoteONmin) MIDINoteOn2=MIDINoteONmax;  // roll over zo maximum
			// MIDINoteOn2=constrain(MIDINoteOn2,MIDINoteONmin,MIDINoteONmax);
			sprintf(sevenSegmentLEDLine, "2c%2d", (MIDINoteOn2-0x8F)); //Show NoteOn2 Channel
			displayLine();
			delay(dispTime);
			}
			
			while (digitalRead(A9)==LOW)
			{
			if (digitalRead(A9)==LOW && digitalRead(A11)==LOW) MIDITranspose2=MIDITranspose2+12; //transpose up
			if (digitalRead(A9)==LOW && digitalRead(A12)==LOW) MIDITranspose2=MIDITranspose2-12; //transpose down
			MIDITranspose2=constrain(MIDITranspose2,MIDITransposemin,MIDITransposemax);
			sprintf(sevenSegmentLEDLine, "2%3d", MIDITranspose2); //Show 2ntranspose
			displayLine();
			delay(dispTime);
			}
			
			if (digitalRead(A10)==LOW)  
			{
				SetUpstate=100; // Ok Button pressed, show all values
				storePiattuinoSettings(SongSetUpNr);
			}
				
			
		} while (SetUpstate==11);
	break;
	
	case 12:		//Test all 30 plates
		sprintf(sevenSegmentLEDLine, "StA2"); //test plate mode
		displayLine();
		delay(dispTime);
	
		for (int i=c5; i<AllPlates; i++)
		{
			makeSound(i);
			//sprintf(sevenSegmentLEDLine, "%2d%2d",i,StrokeTime[i]); //test plate mode
			sprintf(sevenSegmentLEDLine, "%2d%2d",i,StrokeOn); //test plate mode
			displayLine();
			delay(dispTime);
		}
	
		sprintf(sevenSegmentLEDLine, "End2"); 
		displayLine();
		delay(dispTime);
		
		SetUpstate=0;
	break;
	
	case 13:		//Test single plate with Stroke time change
		sprintf(sevenSegmentLEDLine, "StA3"); //test plate mode
		displayLine();
		delay(dispTime);
	
		testPlate=c5;	
		testStrokeOn=defStrokeOn;
		testStrokeOff=defStrokeWait;
		testPlates();
	
		sprintf(sevenSegmentLEDLine, "End3"); 
		displayLine();
		delay(dispTime);

		SetUpstate=0;
	break;
	
	case 14:		//Analyze MIDI song
		sprintf(sevenSegmentLEDLine, "StA4"); //Analyze mode
		displayLine();
		delay(dispTime);
	
		analyzeMIDISound();
		displayResults();
	
		sprintf(sevenSegmentLEDLine, "End4"); 
		displayLine();
		delay(dispTime);
		
		SetUpstate=0;
	break;
	
	case 15:		//Set MIDI - Plate Delay
		sprintf(sevenSegmentLEDLine, "StA5"); //Set Delay Mode
		displayLine();
		delay(dispTime);
	
		do
		{
			if (digitalRead(A11)==LOW) delayMIDI=delayMIDI+25;
			if ((digitalRead(A12)==LOW) && (delayMIDI>0)) delayMIDI=delayMIDI-25;
			delayMIDI=constrain(delayMIDI,delayMIDImin,delayMIDImax);
				
			sprintf(sevenSegmentLEDLine, "D%3d", delayMIDI); //Show current delay
			displayLine();
			delay(dispTime);
			
			if (digitalRead(A10)==LOW)  SetUpstate=0; // Ok Button pressed
			
		} while (SetUpstate==15);
		
		if (delayMIDI==0) delayMIDIOn=false;
		else delayMIDIOn=true;
	
		storePiattuinoSettings(SongSetUpNr);
		 
		sprintf(sevenSegmentLEDLine, "End5"); 
		displayLine();
		delay(dispTime);
		clearDisplay();
		
	break;
	
	case 100:		//Show All Values and Play Test Song if T3 is pressed long
		sprintf(sevenSegmentLEDLine, "1c%2d", (MIDINoteOn1-0x8F)); //Show C -> Channel
		displayLine();
		delay(dispTime);
		sprintf(sevenSegmentLEDLine, "1%3d", MIDITranspose1); //Show t -> transpose
		displayLine();
		delay(dispTime);
		sprintf(sevenSegmentLEDLine, "2c%2d", (MIDINoteOn2-0x8F)); //Show C -> Channel
		displayLine();
		delay(dispTime);
		sprintf(sevenSegmentLEDLine, "2%3d", MIDITranspose2); //Show t -> transpose
		displayLine();
		delay(dispTime);
		
		getSupplyVoltage();
		sprintf(sevenSegmentLEDLine, "b%3d", SupplyVoltageZehntel); //Show Supply Voltage
		displayLine();
		delay(dispTime);
		//sprintf(sevenSegmentLEDLine, "D%3d", delayMIDI); //Show Dealy in ms
		//displayLine();
		//delay(dispTime);
		if (digitalRead(A10)==LOW) 
		{
			sprintf(sevenSegmentLEDLine, "BBS ");
			displayLine();
			playBigBenSound();
		}
		clearDisplay();
		SetUpstate=0;	
	break;
	
	case 200:		//Show song Number
		sprintf(sevenSegmentLEDLine, "So%2d", SongSetUpNr); //Show cureent song nr
		displayLine();
		delay(dispTime);
		
		do
		{
			if (digitalRead(A11)==LOW) ++SongSetUpNr;
			if (digitalRead(A12)==LOW) --SongSetUpNr;
			if (SongSetUpNr<SongSetUpNrmin) SongSetUpNr=SongSetUpNrmax;
			if (SongSetUpNr>SongSetUpNrmax) SongSetUpNr=SongSetUpNrmin;
				
			sprintf(sevenSegmentLEDLine, "So%2d", SongSetUpNr); //Show song nr
			displayLine();
			delay(dispTime);
			
			if (digitalRead(A10)==LOW)  SetUpstate=100; // Ok Button, display all values
			
		} while (SetUpstate==200);
		
		storeSongSetupNr();  
		readPiattuinoSettings(SongSetUpNr);
		checkPiattuinoSettings();
	break;
	
	case 300:		//Send all notes off to MIDI out
		Serial1.flush();
		SendAllNotesOFFToMIDIout();
		
		sprintf(sevenSegmentLEDLine, "%4s", AllSoundOffMsg);
		displayLine();
		delay(dispTime);
		
		clearDisplay();
		SetUpstate=0;	
	break;
	
	default:
		SetUpstate=0;
     break;
	}


	
} // end of while (setUpstate)

clearDisplay();
SetUpstate=0;
		
} // end of if
}
// -----------------------------------------------------------------------------
void SetPlateToneOn(int plateindex)   //plateindex from 0 to AllPlates-1
{


		
TimeNow=millisInt();

//Serial.print("NOn=");
//Serial.println(plateindex);
	
		if ((plateindex >=0) && (plateindex < AllPlates) && (!SolenoidON[plateindex]))
		{
			StrokeStart[plateindex]=TimeNow;
			SolenoidON[plateindex]=true;
			updateSolenoid(plateindex);
			//Serial.print("NOn=");
			//Serial.println(plateindex);
			
		}
		
	
}

// -----------------------------------------------------------------------------
void SetPlateToneOnDelayed(int plateindex)   //plateindex from 0 to AllPlates-1
{


		
TimeNow=millisInt();

//Serial.print("NOn=");
//Serial.println(plateindex);
	
		if ((plateindex >=0) && (plateindex < AllPlates))
		{
			delayStartTim[delBufInPointer]=TimeNow;
			delayedPlate[delBufInPointer]=plateindex;
			delBufInPointer=(delBufInPointer+1)%delayBufSiz;

			//Serial.print("I=");
			//Serial.print(delBufInPointer);
			//Serial.print(" N=");
			//Serial.println(delBufInPointer-delBufOutPointer);
			
			
		}
		
	
}


//-------------------------------------------
// Update Solenoids
//-------------------------------------------

void updateSolenoid(int SolNumber)

{

int PortNr;

if ((SolNumber>=0) && (SolNumber <AllPlates))
	{
	PortNr=SolNumber+SolenoidPortBase;
	digitalWrite(PortNr,SolenoidON[SolNumber]);	
	//Serial.print("PoSon");
	//Serial.println(PortNr);
	//Serial.println(SolenoidON[SolNumber]);
	}
}
//----------------------------------------------------------------------------
// Display Subroutines
//----------------------------------------------------------------------------

// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplay()
{
  Serial2.write(0x76);  // Clear display command
}

// -----------------------------------------------------------------------------
// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightness(byte value)
{
  Serial2.write(0x7A);  // Set brightness command byte
  Serial2.write(value);  // brightness data byte
}
// -----------------------------------------------------------------------------
// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)

void setDecimals(byte decimals)
{
  Serial2.write(0x77);
  Serial2.write(decimals);
}

// -----------------------------------------------------------------------------
void displayLine()
{
for (int i=0; i<4; i++) Serial2.write(sevenSegmentLEDLine[i]);
}
// -----------------------------------------------------------------------------
void makeSound(byte platenote)
{
	SetPlateToneOn(platenote);
	while (SolenoidON[platenote]) ControlAllPlatesToneOff();
	
}
// -----------------------------------------------------------------------------
void testPlates()
{
	
while (!(digitalRead(A8)==LOW && digitalRead(A10)==LOW)) //Exit test plates
{
	if (digitalRead(A8)==LOW && digitalRead(A11)==LOW) //Next higher tone
	{
		++testPlate;
		testPlate=constrain(testPlate,0,AllPlates-1);
		sprintf(sevenSegmentLEDLine, "A%3d", testPlate); //Show Plate nr
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	if (digitalRead(A8)==LOW && digitalRead(A12)==LOW) //Next lower tone
	{
		--testPlate;
		testPlate=constrain(testPlate,0,AllPlates-1);
		sprintf(sevenSegmentLEDLine, "A%3d", testPlate); //Show Plate nr
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	if (digitalRead(A9)==LOW && digitalRead(A11)==LOW) //higher stroke
	{
		++testStrokeOn;
		testStrokeOn=constrain(testStrokeOn,1,40);
		sprintf(sevenSegmentLEDLine, "S%3d", testStrokeOn); //Show stroke time
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	if (digitalRead(A9)==LOW && digitalRead(A12)==LOW) //lower stroke
	{
		--testStrokeOn;
		testStrokeOn=constrain(testStrokeOn,1,40);
		sprintf(sevenSegmentLEDLine, "S%3d", testStrokeOn); //Show stroke time
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	if (digitalRead(A10)==LOW && digitalRead(A11)==LOW) //longer pause
	{
		if (testStrokeOff>=1000) testStrokeOff+=1000;
		else testStrokeOff+=100;
		testStrokeOff=constrain(testStrokeOff,100,9000);
		sprintf(sevenSegmentLEDLine, "%4d", testStrokeOff); //Show stroke time
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	if (digitalRead(A10)==LOW && digitalRead(A12)==LOW) //shorter pause
	{
		if (testStrokeOff<=1000) testStrokeOff-=100;
		else testStrokeOff-=1000;
		testStrokeOff=constrain(testStrokeOff,100,9000);
		sprintf(sevenSegmentLEDLine, "%4d", testStrokeOff); //Show stroke time
		displayLine();
		delay(dispTime);
		testStrokeEnd=millisInt()+testStrokeOff;  //force stroke
	}
	
	TimeNow=millisInt();
	if ((TimeNow-testStrokeEnd)> testStrokeOff)
	{	
		digitalWrite(SolenoidPortBase+testPlate,HIGH);
		delay(testStrokeOn);
		digitalWrite(SolenoidPortBase+testPlate,LOW);
		testStrokeEnd=TimeNow;
	}
	
	
	
	
}

}	

// -----------------------------------------------------------------------------
void playBigBenSound()
{
const int velocity=1000;

	
	makeSound(a5);
	delay(velocity);
	makeSound(f5);
	delay(velocity);
	makeSound(g5);
	delay(velocity);
	makeSound(c5);
	delay(velocity);
	delay(velocity);
	makeSound(c5);
	delay(velocity/2);
	makeSound(g5);
	delay(velocity);
	makeSound(a5);
	delay(velocity/2);
	makeSound(f5);
	delay(3*velocity);
	
	makeSound(c5s);
	delay(2*velocity);
	makeSound(c5s);
	delay(2*velocity);
	makeSound(c5s);
	delay(2*velocity);
	makeSound(c5s);
	delay(2*velocity);
	makeSound(c5s);
	delay(2*velocity);
	makeSound(c5s);
	delay(2*velocity);
	
	
	
			

}
// -----------------------------------------------------------------------------
void analyzeMIDISound()
{
	int i;
	byte recByte;
	byte MIDIChannelNr;
	
	
	for (i=0; i<AllMidiChannels; i++)
	{
		NoteOnChannel[i]=0;
		lowestNote[i]=255;
		highestNote[i]=0;
	}
	
	Serial.println("Start Sound Analyzer");
	MIDIstate=0;
	MIDINoteValid=false;
	
while(digitalRead(A11)==HIGH)	//while not pushed escape button
{	
	if (Serial1.available())
	{
		if (Serial1.available()>62) 
		{ 
			secureAllSolenoidsOff(); // all ouputs to 0
			Serial.println("Overflow"); 
			sprintf(sevenSegmentLEDLine, "E%3d", Serial1.available()); //Error with bytes available
			displayLine();
			while(1);  //Stop processing data
		} 
		
		recByte=Serial1.read();		//read MIDI Data
		Serial1.write(recByte);		//echo to MIDI OUT
		
		if (!(recByte==254))
		{
				
			//if(recByte>0x8F && recByte<0xA0)Serial.println(recByte);
			//Serial.println(recByte);
		
			switch (MIDIstate) 
			{
			case 0: 
				//Serial.println(MIDIstate);
				if ((recByte>=0x90) && (recByte<=0x9F)) 
				{
					MIDIstate=1;		//means note on received
					MIDIChannelNr=recByte;
				}
			break;
			case 1:
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn)) 
				{
					MIDIstate=2;
					MIDINote=recByte;
				}
				else MIDIstate=0;
			break;
			case 2:
				//Serial.println(MIDIstate);
				if (recByte==0) MIDIstate=3;
				else if ((recByte>=lowestStroke)&&(recByte<=highestStroke))
				{
					MIDINoteValid=true;
					//PlateNum=MIDINote-lowestPlateNote+MIDITranspose1;
					MIDIstate=3;
				}
			break;
			case 3:
				//Serial.println(MIDIstate);
				if ((recByte>=lowestNoteOn)&&(recByte<=highestNoteOn))  
				{
					MIDIstate=2;
					MIDINote=recByte;
				}
				else if ((recByte>=0x90) && (recByte<=0x9F)) 
				{
					MIDIstate=1;		//means note on received
					MIDIChannelNr=recByte;
				}
				else MIDIstate=0;
				break;
			//default:
			//break;
			}
		} //end of not recByte==255
	} //end of serial.available()	
		
if 	(MIDINoteValid) 
	{
		i=MIDIChannelNr%16;
		NoteOnChannel[i]=MIDIChannelNr;
		if (MIDINote<lowestNote[i]) lowestNote[i]=MIDINote;
		if (MIDINote>highestNote[i]) highestNote[i]=MIDINote;
		MIDINoteValid=false;
	}

} //end of while	
	
}	
// -----------------------------------------------------------------------------
void displayResults()
{
	int i;
	int tp;
	
	Serial.println("Results Sound Analyzer");
	for (i=0; i<AllMidiChannels; i++)
	{
		if(NoteOnChannel[i]!= 0)
		{
			Serial.print(i);
			Serial.print(": ");
			Serial.print("Ch=");
			Serial.print(NoteOnChannel[i],HEX);
			Serial.print("/");
			Serial.print(NoteOnChannel[i]);
			Serial.print("\t\tLN=");
			Serial.print(lowestNote[i]);
			Serial.print("\tHN=");
			Serial.print(highestNote[i]);
			Serial.print("\tDH-L=");
			Serial.println((highestNote[i]-lowestNote[i]));
		}
	}
	Serial.println("Piattuino Proposal:");
	Serial.println("Piattuino L=60 H=89");
	for (i=0; i<AllMidiChannels; i++)
	{
		if(NoteOnChannel[i]!= 0)
		{
			tp=0;
			while (((lowestNote[i]+tp)<lowestPlateNote) && (tp<36)) tp+=12;
			if (tp==0)
			{
			while (((lowestNote[i]+tp)>=lowestPlateNote+12) && (tp>-36)) tp-=12;
			}
			Serial.print(i);
			Serial.print(": ");
			Serial.print("Ch=");
			Serial.print(NoteOnChannel[i],HEX);
			Serial.print("/");
			Serial.print(NoteOnChannel[i]);
			Serial.print("\tTP=");
			Serial.print(tp);
			Serial.print("\tLN=");
			Serial.print(lowestNote[i]+tp);
			Serial.print("\tHN=");
			Serial.print(highestNote[i]+tp);
			Serial.print("\tDH-L=");
			Serial.println((highestNote[i]-lowestNote[i]));
		}
	}
	
}
// -----------------------------------------------------------------------------
void secureAllSolenoidsOff()
{
	for (int i=0; i<AllPlates; i++) 
	{
		digitalWrite(i+SolenoidPortBase,0); // all ouputs to 0
		SolenoidON[i]=false;
	}
}
// -----------------------------------------------------------------------------

// Checks Piattuino Settings within allowed ranges (Channel, Transpose and MIDI Delay)

void checkPiattuinoSettings() 

{
	
	if((MIDINoteOn1 < MIDINoteONmin) || (MIDINoteOn1 > MIDINoteONmax) ||
	   (MIDINoteOn2 < MIDINoteONmin) || (MIDINoteOn2 > MIDINoteONmax) ||
	   (MIDITranspose1 < MIDITransposemin) || (MIDITranspose1 > MIDITransposemax) ||
	   (MIDITranspose2 < MIDITransposemin) || (MIDITranspose2 > MIDITransposemax) ||
	   (delayMIDI > delayMIDImax))
	   {		//set all default values because one of it is out of legal range
		   MIDINoteOn1=defMIDINoteOn1;
		   MIDINoteOn2=defMIDINoteOn2;
		   MIDITranspose1=defMIDITranspose1;
		   MIDITranspose2=defMIDITranspose2;
		   delayMIDI=defdelayMIDI;
		   if (delayMIDI==0) delayMIDIOn=false;
		   else delayMIDIOn=true;
		   storePiattuinoSettings(SongSetUpNr);	//write defaults to persistent memory
	   }
		   
}

// -----------------------------------------------------------------------------

// Reads the curren song setup nr

void readSongSetupNr() 

{
	
SongSetUpNr=EEPROM.read(0);
if 	((SongSetUpNr<SongSetUpNrmin) || (SongSetUpNr>SongSetUpNrmax))   //check for valid range
{
	SongSetUpNr=defSongSetUpNr;
	storeSongSetupNr();
}
	
}

// Writes to persistent the curren song setup nr

void storeSongSetupNr() 

{
	
EEPROM.write(0,SongSetUpNr);
	
}
	
// -----------------------------------------------------------------------------

// Reads Channel, Transpose and MIDI Delay Settings from persistent memory
// indicated by SetNr

void readPiattuinoSettings(byte SetNr) 

{

byte Snr;
byte Ind;
Snr=constrain(SetNr,SongSetUpNrmin,SongSetUpNrmax);  //we allow only this range
Ind=Snr*8;				//we have 8 byte for each SetNr

int x;
int y;

MIDINoteOn1=EEPROM.read(Ind+0);		// location 0 MIDI Note ON 1
MIDINoteOn2=EEPROM.read(Ind+1);		// location 1 MIDI Note ON 2

y=EEPROM.read(Ind+2);		// location 2 + 3 MIDI Transpose 1
x=EEPROM.read(Ind+3);		
y+= x << 8;
MIDITranspose1=y;

y=EEPROM.read(Ind+4);		// location 4 + 5 MIDI Transpose 2
x=EEPROM.read(Ind+5);		
y+= x << 8;
MIDITranspose2=y;

y=EEPROM.read(Ind+6);		// location 6 + 7 MIDI Delay
x=EEPROM.read(Ind+7);		// 
y+= x << 8;

delayMIDI=y;

}

//-----------------------------------------------------------------------
// Writes Piattuino Setup Values to persistent memory
// indicated by SetNr

void storePiattuinoSettings(byte SetNr)

{
	
byte Snr;
byte Ind;
Snr=constrain(SetNr,SongSetUpNrmin,SongSetUpNrmax);  //we allow only this range
Ind=Snr*8;				//we have 8 byte for each SetNr

EEPROM.write(Ind+0,MIDINoteOn1); // NoteOn1 to location 0
EEPROM.write(Ind+1,MIDINoteOn2); // NoteOn2 to location 1

EEPROM.write(Ind+2, lowByte(MIDITranspose1)); 	// location 2 + 3 MIDI Transpose 1
EEPROM.write(Ind+3, highByte(MIDITranspose1));	

EEPROM.write(Ind+4, lowByte(MIDITranspose2)); 	// location 4 + 5 MIDI Transpose 2
EEPROM.write(Ind+5, highByte(MIDITranspose2));	

EEPROM.write(Ind+6, lowByte(delayMIDI));		// location 6 + 7 MIDI Delay
EEPROM.write(Ind+7, highByte(delayMIDI));


}

//-----------------------------------------------------------------------
/* Speisespannung messen
   Regeln:
   AD-Wert: 1023 - > 15,16 V
   AD-Wert: LS-Bit (1 Bit) -> 15,16 / 1023 = 14,82 mV
   */
//----------------------------------------------------------------------------

void getSupplyVoltage()
{
 int Vbatt = analogRead(batteryVoltageInput);
 int x;
 //Serial.print('(');
 //Serial.print(Vbatt);
 //Serial.print(") ");
 
 SupplyVoltage= Vbatt * .01482;	//converting from a 0 to 1023 digital range
                                // to 0 to 15,16 Volts (each 1 reading equals ~ 14,82 millivolts
 
 
 x = int(SupplyVoltage * 100);		
 SupplyVoltageZehntel = x/10;
 if (x % 10 > 4) SupplyVoltageZehntel++;
 if (x % 10 < -4) SupplyVoltageZehntel--;
  
 }

//----------------------------------------------------------------------------

void SendAllNotesOFFToMIDIout()
{
	
	for (int i=0; i<VPLen; i++)  Serial1.write(VolcanoPanic[i]);  //Notes Off string to MIDI out
	
	
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

void setup() {
   
for (int i=SolenoidPortBase; i<SolenoidPortBase+AllPlates; i++) 
  {	
	pinMode(i,OUTPUT);  //Ports 22-51 are the Solenoid Ouputs
	digitalWrite(i,0); // all ouputs to 0
  }

pinMode(A8,INPUT_PULLUP);   //Button 1
pinMode(A9,INPUT_PULLUP); 	//Botton 2
pinMode(A10,INPUT_PULLUP); 	//Button 3
pinMode(A11,INPUT_PULLUP);  //Switch ESC
pinMode(A12,INPUT_PULLUP);  //Switch Ok


// the console
Serial.begin(115200);
Serial.print("Piattuino ");
Serial.print(Version);
Serial.println(Release);

//get the supply voltage
getSupplyVoltage();
   
Serial.print("Power = ");
Serial.print(SupplyVoltage);
Serial.print("V");
Serial.print(" (");
Serial.print(SupplyVoltageZehntel);
Serial.println(") ");

	
// The Midi Baud Rate is 31250
Serial1.begin(31250);
Serial1.flush();
SendAllNotesOFFToMIDIout();
	  
  for (int i=0; i< AllPlates; i++)
  {
	  StrokeStart[i]=0;
	  SolenoidON[i]=false;
  }

readSongSetupNr();  
readPiattuinoSettings(SongSetUpNr);
checkPiattuinoSettings();

// The Numeric Display
Serial2.begin(9600);
clearDisplay();  // Clears display, resets cursor
setBrightness(255);  // High brightness


//Show piattuino settings
sprintf(sevenSegmentLEDLine, "%4s", SegDispVersion); //Version and release 
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "b%3d", SupplyVoltageZehntel); //Show Supply Voltage
displayLine();
delay(dispTime);

sprintf(sevenSegmentLEDLine, "So%2d", SongSetUpNr); //Show song nr
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "1c%2d", (MIDINoteOn1-0x8F)); //Show C -> Channel
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "1%3d", MIDITranspose1); //Show t -> transpose
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "2c%2d", (MIDINoteOn2-0x8F)); //Show C -> Channel
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "2%3d", MIDITranspose2); //Show t -> transpose
displayLine();
delay(dispTime);
sprintf(sevenSegmentLEDLine, "D%3d", delayMIDI); //Show Dealy in ms
displayLine();
delay(dispTime);			
			
clearDisplay();  // Clears display, resets cursor

Serial.println("Ready");
		
}

void loop() 
{
	GetMIDITone();
	if (delayMIDIOn) ControlDelayedPlatesToneOn();
	ControlAllPlatesToneOff();
	ControlAllSwitches();
	
		
}

// -----------------------------------------------------------------------------
