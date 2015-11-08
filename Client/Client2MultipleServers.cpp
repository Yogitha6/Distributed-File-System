#include "md5.h"
#include <fstream>
#include <string>
#include <cstdlib>
#include<iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <time.h>
using namespace std;


////////////////////////////////    Encryption Section of the code  ////////////////////////////////


static inline unsigned int value(char c)
{
    if (c >= '0' && c <= '9') { return c - '0';      }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
}

std::string str_xor(std::string const & s1, std::string const & s2)
{
    static char const alphabet[] = "0123456789abcdef";

    std::string result;
    result.reserve(s1.length());

    for (std::size_t i = 0; i != s1.length(); ++i)
    {
        unsigned int v = value(s1[i]) ^ value(s2[i]);

        result.push_back(alphabet[v]);
    }

    return result;
}

std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string hex_to_string(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2)
    {
        char a = input[i];
        const char* p = std::lower_bound(lut, lut + 16, a);

        char b = input[i + 1];
        const char* q = std::lower_bound(lut, lut + 16, b);

        output.push_back(((p - lut) << 4) | (q - lut));
    }
    return output;
}

std::string encryptText(string password,string text)
{
    if(password.length()!=text.length())
    {
            password = password.substr(0,text.length());
    }
    string operand1 = string_to_hex(password);
    string operand2 = string_to_hex(text);
    string operand1_xor_operand2 = str_xor(operand1,operand2);
    return operand1_xor_operand2;
}

std::string decryptText(string password,string ciphertext,int length)
{
    password = password.substr(0,length);
    string operand1 = string_to_hex(password);
    string operand1_xor_operand2 = str_xor(operand1,ciphertext);
    return hex_to_string(operand1_xor_operand2);
}

////////////////////////////////    Functions to help the Distributed File system  ////////////////////////////////

int convertToInt(string hexvalue)
{
    unsigned long intvalue = strtoul(hexvalue.c_str(), 0, 16);
    return intvalue;
}
int md5HashMod4(string filename)
{
    string line,fileContent;
    ifstream myfile(filename.c_str());
    if(myfile.is_open())
    {
        while(getline(myfile,line))
        {
            fileContent = fileContent + line;
            fileContent = fileContent + '\n';
        }
        myfile.close();
    }
    else
        cout<<"unable to open the file";
    string md5hash = md5(fileContent);
    string moduloHash = md5hash.substr(md5hash.length()-2);
    int modulusValue = convertToInt(moduloHash)%4;
    return modulusValue;
}

int divideFile(string filename)
{
    ifstream bigfile;
    bigfile.open(filename.c_str(),ios::in | ios::binary);

    // get length of file:
    bigfile.seekg (0, ios::end);
    int length = bigfile.tellg();
    bigfile.seekg (0, ios::beg);

    int achunk = length/4;
       vector<char> buffer (achunk);

       for (int i = 0; i < 4; i++)
        {
         //Build File Name
        string partname = filename;
        string charnum;     //archive number
        stringstream out;
        out << "." << i;
        charnum = out.str();
        partname.append(charnum);  //put the part name together

        //chunking and writing to small files
        bigfile.read (&buffer[0], achunk);
        ofstream smallfile;
        smallfile.open(partname.c_str(),ios::out | ios::binary);
        smallfile.write (&buffer[0], achunk);
        smallfile.close();
        }
}

/////////////////////   Function to check the Authenticity with the DFS servers  //////////////
bool checkConnection(int listenFd[],string username, string password)
{
    bool authenticated[4] = {false,false,false,false};
    char test[300];
    bzero(test, 301);
    for(int j = 0; j<4; j++)
    {
        write(listenFd[j],"-o CHK ",6);
        write(listenFd[j],"\r\n",2);

        write(listenFd[j],username.c_str(),strlen(username.c_str()));
        write(listenFd[j],"\r\n",2);

        write(listenFd[j],password.c_str(),strlen(password.c_str()));
        write(listenFd[j],"\r\n",2);

        while(true)
        {

         bzero(test, 301);
        recv(listenFd[j],test, 300,0);
        string tester (test);
        cout << "value got from DFS"<<j<<" is "<<tester<< endl;

         if(tester.find("authenticated")==std::string::npos)
            {
            authenticated[j] = false;
            return false;
            }
        break;
        }
    }
    return true;
}

/////////////////////   Function to List the files from the DFS servers  //////////////

void split(const string& s, char c,
           vector<string>& v) {
   string::size_type i = 0;
   string::size_type j = s.find(c);

   while (j != string::npos) {
      v.push_back(s.substr(i, j-i));
      i = ++j;
      j = s.find(c, j);

      if (j == string::npos)
         v.push_back(s.substr(i, s.length()));
   }
}

bool _find_(vector<string> &listofFiles,string filename)
{
bool found = false;

for(int i = 0; i < listofFiles.size(); i ++)
{
    if(listofFiles[i].find(filename)!=std::string::npos)
    {
        found = true;
        break;
    }
}
return found;
}

int countNumberofUniqueFiles(vector<string> files)
{
    int count = 0;
    int totalsize = 0;
    vector<string> listofUniqueFileNames;

    for(int i = 0; i<files.size(); i++)
    {
        while(files[i].find("txt")!=std::string::npos)
        {
        std::size_t pos = files[i].find(".txt");
        std::size_t pos_delimiter;
        if(files[i].find("../")!=std::string::npos)
        {
            pos_delimiter = files[i].find("../");
        }
        else
        {
            pos_delimiter = files[i].find("/");
        }
        if(_find_(listofUniqueFileNames,files[i].substr(pos_delimiter+1,pos+2))==false)
        {
            listofUniqueFileNames.push_back(files[i].substr(pos_delimiter+1,pos+2));
        }
        files[i] = files[i].substr(pos+2);
        }
    }
    vector<string> listofUniqueFileNames1;
    for(int p = 0; p<listofUniqueFileNames.size(); p++)
    {
        std::size_t pos = listofUniqueFileNames[p].find(".txt");
        std::size_t pos_delimiter = listofUniqueFileNames[p].find("/");
        if(_find_(listofUniqueFileNames1,listofUniqueFileNames[p].substr(0,pos+2))==false)
        {
            listofUniqueFileNames1.push_back(listofUniqueFileNames[p].substr(0,pos+2));
            count = count + 1;
        }
    }
    return count;
}

int combineAllFilesfromServers(vector<string>& files,vector<string>& allfilesfrom_Servers)
{
    int numberofUniqueFilenames = countNumberofUniqueFiles(files);

    for(int j=0; j<files.size(); j++)
    {
        vector<string> v;
        split(files[j], '/', v);
        int filled = 0;
        if (v.size()-3 < numberofUniqueFilenames * 2)
        {
            for (int i = 0; i < v.size(); ++i)
            {
            char temp = v[i].at(0);
            if(temp!= '.' && temp!='e')
            {
                allfilesfrom_Servers.push_back(v[i]);
                filled = filled + 1;
            }
            }
            while(filled<numberofUniqueFilenames*2)
            {
                allfilesfrom_Servers.push_back("dummyZ.txt");
                filled = filled+1;
            }
        }
        else
        {
             for (int i = 0; i < v.size(); ++i)
            {
            char temp = v[i].at(0);
            if(temp!= '.' && temp!='e')
            {
                allfilesfrom_Servers.push_back(v[i]);
            }
            }
        }
    }
    return numberofUniqueFilenames;
}

void getFileOptimally(string filename, vector<string>& files, vector<int>& filePlacement)
{
    vector<string> allfilesfrom_Servers;
    int numberofUniqueFiles = combineAllFilesfromServers(files,allfilesfrom_Servers);

    vector<string> fileParts;
    fileParts.push_back(filename+".0");
    fileParts.push_back(filename+".1");
    fileParts.push_back(filename+".2");
    fileParts.push_back(filename+".3");
    int j = 0;
    int totalsize = allfilesfrom_Servers.size();
    int serverBoundary = numberofUniqueFiles*2;
    int noOfLoops = totalsize;
    while(filePlacement.size()<4 and noOfLoops>0)
    {
    for(int i = 0; i < allfilesfrom_Servers.size(); i ++)
    {
        if(allfilesfrom_Servers[i]==fileParts[j])
        {
            filePlacement.push_back((i/(2*numberofUniqueFiles)) + 1);
            j++;
            if(j==4)
                break;
        }
    }
    noOfLoops = noOfLoops -1;
    }
    if(filePlacement.size()!=4)
    {
        cout<<filename<<" :[Incomplete]"<<endl;
    }
}

void checkCompletenessOfAllFiles(vector<string>& files)
{
    vector<string> allfilesfrom_Servers;
    int numberofUniqueFiles = combineAllFilesfromServers(files,allfilesfrom_Servers);

    int totalsize = allfilesfrom_Servers.size();
    int serverBoundary = numberofUniqueFiles*2;

    vector<string> listofUniqueFileNames;

    for(int i = 0; i < totalsize; i++)
    {
        std::size_t pos = allfilesfrom_Servers[i].find(".txt");
        if(_find_(listofUniqueFileNames,allfilesfrom_Servers[i].substr(0,pos+4))==false)
        {
            listofUniqueFileNames.push_back(allfilesfrom_Servers[i].substr(0,pos+4));
        }
    }

    for(int fileNumber = 0; fileNumber < numberofUniqueFiles; fileNumber++)
    {
        if(listofUniqueFileNames[fileNumber].find("Z")==std::string::npos)
        {
        vector<string> fileParts;
        fileParts.push_back(listofUniqueFileNames[fileNumber]+".0");
        fileParts.push_back(listofUniqueFileNames[fileNumber]+".1");
        fileParts.push_back(listofUniqueFileNames[fileNumber]+".2");
        fileParts.push_back(listofUniqueFileNames[fileNumber]+".3");
        bool complete = true;

        for(int p = 0; p<fileParts.size(); p++)
        {
            if(_find_(allfilesfrom_Servers,fileParts[p])!=true)
            {
                cout<< listofUniqueFileNames[fileNumber] <<" [Incomplete] "<<endl;
                complete = false;
                break;
            }
        }
        if(complete == true)
            cout<< listofUniqueFileNames[fileNumber] <<" [Complete] "<<endl;
        }
    }
}


void listfromServer(int listenFd[],bool get, string filename,vector<int>& filePlacement)
{
    vector<string> files = vector<string>();
    char test[300];
    bzero(test, 301);

    string filename_temp = filename;
    string dirname = "";
            if(filename.find("/")!=std::string::npos)
            {
                size_t pos = filename.find("/");
                dirname = filename.substr(0,pos+1);
                filename_temp = filename.substr(pos+1);
            }

    for(int j = 0; j< 4; j++)
    {
        write(listenFd[j],"-o LIS",6);
        //write(listenFd[j],"\r\n",2);
        write(listenFd[j],dirname.c_str(),strlen(dirname.c_str()));
        write(listenFd[j],"\r\n",2);

        sleep(1);
        int filecount = 0;
        while(true)
        {
        bzero(test, 301);
        read(listenFd[j],test, 300);
        string tester (test);

        if(tester.find(".txt")!= std::string::npos)
        {
           files.push_back(tester);
        }
        if(tester.find("exit") !=std::string::npos)
            break;
        if(tester.substr(0,1) == " ")
            break;
        if(tester == "")
            break;
        }
    }

     if(get == true)
        {
        getFileOptimally(filename_temp,files,filePlacement);
        }
    else
        {
        checkCompletenessOfAllFiles(files);

         for(int j = 0;j < 4; j ++)
        {
            write(listenFd[j],"exit",4);
            write(listenFd[j],"\r\n",2);
        }
        }
}


/////////////////////   Function to get the file from the DFS servers  //////////////

void getfromServer(int listenFd[],string filename, string password)
{
    char test[300];
    bzero(test, 301);

    vector<int> filePlacement;

    cout<<"Optimal way to get the four parts of the file are "<<endl;
    listfromServer(listenFd,true,filename,filePlacement);

    for(int t = 0; t<filePlacement.size(); t++)
        cout<<filePlacement[t]<<endl;

    if(filePlacement.size()==4)
    {
    for(int j = 0; j< 4; j++)
    {
        stringstream ss;
        ss << j;
        string filepart = filename+"."+ss.str();
        int servertoContact = filePlacement[j];
                cout<<"asking file "<<filepart<<" from the DFS"<<servertoContact<<endl;
        write(listenFd[servertoContact-1],"-o GET",6);
        write(listenFd[servertoContact-1],"\r\n",2);
        write(listenFd[servertoContact-1],filepart.c_str(),strlen(filepart.c_str()));
        write(listenFd[servertoContact-1],"\r\n",2);

        string filename_temp;
        while(true)
        {
        bzero(test, 301);
        recv(listenFd[servertoContact-1],test, 300,0);
        string tester1 (test);

        if(tester1.find(".txt")!=std::string::npos or tester1.find("content")!=std::string::npos)
        {
            std::size_t pos = tester1.find(".txt");
            std::size_t pos_content = tester1.find("content");
            if(pos<tester1.size())
            {
                filename_temp = tester1.substr(pos-5,11)+"DFS";
            }
            if(pos_content<tester1.size())
            {
            string valuetoWrite = tester1.substr(pos_content+9);
            ofstream smallfile;
            smallfile.open(filename_temp.c_str(),ios::out | ios::binary);
            smallfile.write(valuetoWrite.c_str(),strlen(valuetoWrite.c_str()));
            smallfile.close();
            }
        }

        if(tester1.find("exit") !=std::string::npos)
            break;
        if(tester1.substr(0,1) == " ")
            break;
        if(tester1 == "")
            break;
        }
    }

    ofstream bigfile;
    string newfile = filename+"_fileBuiltfromDFS.txt";
            string finalbuffer;

    //reconstructing the file
    for(int fileNumber=0; fileNumber <4; fileNumber ++)
    {
        string oldfile = filename+".";
        stringstream convert1;
        convert1<<fileNumber;
        oldfile = oldfile+ convert1.str()+"DFS";
        ifstream file(oldfile.c_str());
        string linebuffer;
        while (file && getline(file, linebuffer))
             {
                if(linebuffer.length() == 0)break;
                if(linebuffer.find("exit")!= std::string::npos) break;
                if(linebuffer.length()>1)
                {
                linebuffer = decryptText(password,linebuffer,linebuffer.length()/2);
                cout<<"decrypted "<<linebuffer<<endl;
                finalbuffer+=linebuffer;
                finalbuffer+="\n";
                }
                else
                {
                    break;
                }
             }
       //cout<<"got from file "<<oldfile<<" and writing to the new file "<<finalbuffer<<endl;
        }
       bigfile.open(newfile.c_str(),ios::out | ios::binary);
       bigfile.write(finalbuffer.c_str(),strlen(finalbuffer.c_str()));
       bigfile.close();
    }

    for(int j = 0;j < 4; j ++)
        {
                //write(listenFd[j], " ",1);
            write(listenFd[j],"exit",4);
            write(listenFd[j],"\r\n",2);
                //write(listenFd[j],"\r\n",2);
        }
}


/////////////////////   Function to PUT the file into the DFS servers  //////////////
void writeToServer(int listenFd[],string filename, int hashvalue, string password)
{
    int DFS1[4][2] = {{1,2},{4,1},{3,4},{2,3}};
    int DFS2[4][2] = {{2,3},{1,2},{4,1},{3,4}};
    int DFS3[4][2] = {{3,4},{2,3},{1,2},{4,1}};
    int DFS4[4][2] = {{4,1},{3,4},{2,3},{1,2}};
    string filename_temp;
    for(int k = 0; k <2; k ++)
    {
   //for writing the file into the server
        for(int j = 0;j < 4; j ++)
        {
            filename_temp = filename;
            string dirname = "";
            if(filename.find("/")!=std::string::npos)
            {
                size_t pos = filename.find("/");
                dirname = filename.substr(0,pos);
                filename_temp = filename.substr(pos+1);
            }
            string Result;//string which will contain the result

            stringstream convert; // stringstream used for the conversion

            if(j==0)
            {
                convert << DFS1[hashvalue][k] - 1;//add the value of Number to the characters in the stream
            }
            else if(j==1)
            {
                convert << DFS2[hashvalue][k] - 1;//add the value of Number to the characters in the stream
            }
            else if(j==2)
            {
                convert << DFS3[hashvalue][k] - 1;//add the value of Number to the characters in the stream
            }
            else if(j==3)
            {
                convert << DFS4[hashvalue][k] - 1;//add the value of Number to the characters in the stream
            }

            Result = convert.str();
            filename_temp = filename_temp + "."+Result;

            ifstream file(filename_temp.c_str());
            string linebuffer;
            string finalbuffer;
            while (file && getline(file, linebuffer))
                {
                    if (linebuffer.length() == 0)continue;
                    linebuffer = encryptText(password,linebuffer);
                    finalbuffer+=linebuffer;
                    finalbuffer+="\n";
                }
            if (dirname.size()>1)
                filename_temp = dirname+"/"+filename_temp;

            cout<<"File Content being written to DFS" <<j <<endl<<" "<<finalbuffer<<endl;
            write(listenFd[j],"-o PUT",6);
            write(listenFd[j],"\r\n",2);
            write(listenFd[j],filename_temp.c_str(),strlen(filename_temp.c_str()));
            write(listenFd[j],"\r\n",2);
            write(listenFd[j],finalbuffer.c_str(),strlen(finalbuffer.c_str()));
            write(listenFd[j],"\r\n",2);
            sleep(2);
        }
    }

        //for closing the connections
        for(int j = 0;j < 4; j ++)
        {
                //write(listenFd[j], " ",1);
                write(listenFd[j],"exit",4);
                write(listenFd[j],"\r\n",2);
                //write(listenFd[j],"\r\n",2);

        }
}

int main (int argc, char* argv[])
{
    string portArray[] = {"","","",""};
    int listenFd[4];

/////////////////////   Reading the port numbers of DFS servers from the dfc.conf file  //////////////
    string filename = "dfc.conf";
    ifstream file(filename.c_str());
    string linebuffer;
    string finalbuffer;

    int count = 0;
    while (file && getline(file, linebuffer))
    {
        if (linebuffer.length() == 0)continue;
        if(count<4)
           {
             portArray[count]=linebuffer.substr(22);
           }
        count = count + 1;
    }

/////////////////////   CONNECTING TO THE FOUR DFS SERVERS   /////////////////////////////////////////
    for(int i = 0; i < 4; i ++)
    {
        int portNo;
        bool loop = false;
        struct sockaddr_in svrAdd;
        struct hostent *server;

        portNo = atoi(portArray[i].c_str());
        if((portNo > 65535) || (portNo < 2000))
        {
            cerr<<"Please enter port number between 2000 - 65535"<<endl;
            return 0;
        }
        //create client socket
        listenFd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if(listenFd[i] < 0)
        {
            cerr << "Cannot open socket" << endl;
            return 0;
        }
        server = gethostbyname("localhost");
        if(server == NULL)
        {
            cerr << "Host does not exist" << endl;
            return 0;
        }
        bzero((char *) &svrAdd, sizeof(svrAdd));
        svrAdd.sin_family = AF_INET;
        bcopy((char *) server -> h_addr, (char *) &svrAdd.sin_addr.s_addr, server -> h_length);
        svrAdd.sin_port = htons(portNo);
        int checker = connect(listenFd[i],(struct sockaddr *) &svrAdd, sizeof(svrAdd));
        if (checker < 0)
        {
            cerr << "Cannot connect!" << endl;
            return 0;
        }
    }

    ////////////////////////////////////////// PUT Section of CODE CLIENT PROCESSING   ////////////////////////////////////////////////
    string password;
    string username;
    string linebuffer1;
    string confFile= "dfc.conf";
    ifstream file1(confFile.c_str());
    while (file1 && getline(file1, linebuffer1))
    {
        if (linebuffer1.length() == 0)continue;
        if(linebuffer1.find("Password")!=std::string::npos)
            {
                password = linebuffer1.substr(10);
            }
        if(linebuffer1.find("Username")!=std::string::npos)
        {
            username = linebuffer1.substr(10);
        }
    }
    cout<<"username is "<<username<<endl;
    cout<<"password is "<<password<<endl;

    bool Authenticated = checkConnection(listenFd,username,password);
    if(Authenticated==true)
    {
    if(std::string(argv[1])=="PUT")
    {
    string filename = argv[2];
    if(filename.find("/")!=std::string::npos)
    {
        size_t pos = filename.find("/");
        filename = filename.substr(pos+1);
    }
    cout<<"file name is "<<filename<<endl;
    divideFile(std::string(filename));
    cout <<"Hash value lookup for table "<<md5HashMod4(std::string(filename))<<endl;
    writeToServer(listenFd,std::string(argv[2]),md5HashMod4(std::string(filename)),password);
    }
    else if(std::string(argv[1])=="GET")
    {
    getfromServer(listenFd,std::string(argv[2]),password);
    }
    else if(std::string(argv[1])=="LIS")
    {
    string filename = "dummy";
    vector<int> dummy;
    if(argv[2]!=NULL)
    {
        filename = string(argv[2]);
    }
    listfromServer(listenFd,false,filename,dummy);
    }
    }
    else
    {
        cout<<"“Invalid	Username/Password.	Please	try	again";
    }
}
