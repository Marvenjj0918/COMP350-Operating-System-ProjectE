void printString(char* s);
void printChar(char c);
void readString(char* line);
void readSector(char* buffer, int sector);
void readFile(char* fileName,char* buffer,int* readSectors);
void executeProgram(char* fileName);
void terminate();
void fnf();
void handleTimerInterrupt(int segment, int sp);
void handleInterrupt21(int ax, int bx, int cx, int dx);

int processActive[8];
int processStackPointer[8];
int currentProcess;

main()
{
	int i,j;
	for(i=0;i<8;i++)
		processActive[i]=0;
	for(j=0;j<8;j++)
		processStackPointer[j]=0xff00;
	makeInterrupt21();
	currentProcess=-1;
	interrupt(0x21,4,"shell");
	makeTimerInterrupt();
	while(1);
}

void printString(char* chars)
{
	int i=0;
	while(chars[i]!='\0')
		printChar(chars[i++]);
}

void printChar(char c)
{
	interrupt(0x10,0x0e*0x100+c,0,0);
}

void readString(char* line)
{
	int i=0;
	char c;
	while(1)
	{
		c=interrupt(0x16,0,0,0,0);
		if(c==0x8)
		{
			if(i>0)
			{
				i--;
				printChar(0x8);
				printChar(' ');
				printChar(0x8);
			}
		}
		else if(c==0xd)
			break;
		else
		{
			line[i++]=c;
			printChar(c);
		}
	}
	line[i++]=0xd;
	line[i++]=0xa;
	line[i++]=0;
	printChar(0xd);
	printChar(0xa);
}

void readSector(char* buffer, int sector)
{
	int AH=2;
	int AL=1;
	int BX=buffer;
	int CH=0;
	int CL=sector+1;
	int DH=0;
	int DL=0x80;
	int AX=AH*256+AL;
	int CX=CH*256+CL;
 	int DX=DH*256+DL;
	interrupt(0x13,AX,BX,CX,DX);
}

void readFile(char* fileName,char* buffer,int* readSectors)
{
	int i,k,temp;
	char directory[512];
	readSector(directory, 2);
	for(i=0; i<512; i=i+32)
	{
		temp=i;
		if((fileName[0]==directory[temp])&&(fileName[1]==directory[temp+1])&&(fileName[2]==directory[temp+2])&&(fileName[3]==directory[temp+3])&&(fileName[4]==directory[temp+4])&&(fileName[5]==directory[temp+5]))
		{
			k=i+6;
			while(directory[k]!=0)
			{
				readSector(buffer,directory[k]);
				k++;
				*readSectors=*readSectors+1;
			}
			return;
		}
	}
}

void executeProgram(char* fileName)
{
	int i=0,j=0;
	int segment=0;
	int dataseg=0;
	int readSectors;
	char buffer1[13312];
	readSectors=0;
	interrupt(0x21,3,fileName,buffer1,&readSectors);
	if(readSectors!=0)
	{
		dataseg = setKernelDataSegment();
		while(processActive[j]!=0)
		{
			j++;
			if(j==8)
				j=0;
		}
		restoreDataSegment(dataseg);
		segment=(j+2)*0x1000;
		for(i=0;i<13312;i++)
		{
			putInMemory(segment,i,buffer1[i]);
		}
		initializeProgram(segment);
		dataseg = setKernelDataSegment();
		processStackPointer[j]=0xff00;
		processActive[j]=1;
		restoreDataSegment(dataseg);
	}
	else
	{
		fnf();
		return;
	}
}

/*
void executeProgram(char* fileName)
{
	int i;
	int seg=20000;
	int readSectors;
	char buffer1[13312];
	readSectors=0;
	interrupt(0x21,3,fileName,buffer1,&readSectors);
	if(readSectors!=0)
	{
		for(i=0;i<13312;i++)
		{
			putInMemory(seg,i,buffer1[i]);
		}
		launchProgram(seg);
	}
	else
	{
		fnf();
	}
}
*/

void terminate()
{
	int dataseg=0;
	dataseg=setKernelDataSegment();
	processActive[currentProcess]=0;
	restoreDataSegment(dataseg);
	while(1);
}

void fnf()
{
	char error[15];
	error[0]='F';
	error[1]='i';
	error[2]='l';
	error[3]='e';
	error[4]=' ';
	error[5]='N';
	error[6]='o';
	error[7]='t';
	error[8]=' ';
	error[9]='F';
	error[10]='o';
	error[11]='u';
	error[12]='n';
	error[13]='d';
	error[14]='\0';
	printString(error);
	return;
}

void handleTimerInterrupt(int segment, int sp)
{
	int dataseg=0;
	int j=0;
	//printChar('T');
	//printChar('I');
	//printChar('C');
	dataseg = setKernelDataSegment();
	if(currentProcess!=-1)
		processStackPointer[currentProcess]=sp;
	j=currentProcess;
	j++;
	if(j==8)
		j=0;
	while(processActive[j]!=1)
	{
		j++;
		if(j==8)
			j=0;
	}
	currentProcess=j;
	segment=(currentProcess+2)*0x1000;
	sp=processStackPointer[currentProcess];
	restoreDataSegment(dataseg);
	returnFromTimer(segment,sp);
}

void handleInterrupt21(int ax, int bx, int cx, int dx)
{
	if(ax==0)
		printString(bx);
	else if(ax==1)
		readString(bx);
	else if(ax==2)
		readSector(bx,cx);
	else if(ax==3)
		readFile(bx,cx,dx);
	else if(ax==4)
		executeProgram(bx);
	else if(ax==5)
		terminate();
	else
		printString("Unhandled Interrupt 21");
}
