#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
using namespace std;

void *task1(void *);

static int connFd;

int main(int argc, char* argv[])
{
    int pId, portNo, listenFd;
    socklen_t len; //store size of the address
    bool loop = false;
    struct sockaddr_in svrAdd, clntAdd;

    pthread_t threadA[3];

    if (argc < 2)
    {
        cerr << "Syntam : ./server <port>" << endl;
        return 0;
    }

    portNo = 10003;

    if((portNo > 65535) || (portNo < 2000))
    {
        cerr << "Please enter a port number between 2000 - 65535" << endl;
        return 0;
    }

    //create socket
    listenFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(listenFd < 0)
    {
        cerr << "Cannot open socket" << endl;
        return 0;
    }

    bzero((char*) &svrAdd, sizeof(svrAdd));

    svrAdd.sin_family = AF_INET;
    svrAdd.sin_addr.s_addr = INADDR_ANY;
    svrAdd.sin_port = htons(portNo);

    //bind socket
    if(bind(listenFd, (struct sockaddr *)&svrAdd, sizeof(svrAdd)) < 0)
    {
        cerr << "Cannot bind" << endl;
        return 0;
    }

    listen(listenFd, 5);

    int noThread = 0;

    while (noThread < 5)
    {
	socklen_t len = sizeof(clntAdd);
        cout << "Listening" << endl;

        //this is where client connects. svr will hang in this mode until client conn
        connFd = accept(listenFd, (struct sockaddr *)&clntAdd, &len);

        if (connFd < 0)
        {
            cerr << "Cannot accept connection" << endl;
            return 0;
        }
        else
        {
            cout << "Connection successful" << endl;
        }

        pthread_create(&threadA[noThread], NULL, task1, NULL);

        noThread++;
    }

    for(int i = 0; i < 5; i++)
    {
        pthread_join(threadA[i], NULL);
    }


}

int getFilesfromDirectory(string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if(string(dirp->d_name).find(".txt")!=std::string::npos)
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

int createDirectory(string dirname)
{
    cout<<"Creating a Directory "<<dirname<<endl;
    int result = mkdir(dirname.c_str(), 0777);
    return result;
}

void *task1 (void *dummyPt)
{
    cout << "Thread No: " << pthread_self() << endl;
    char test[300];
    bzero(test, 301);
    bool loop = false;
    int count = 0;
    string filename;
    bool put = false;
    bool get = false;
    bool lis = false;
    bool chk = false;

    while(!loop)
    {
        bzero(test, 301);

        string filename_temp = filename;
        recv(connFd, test, 300,0);

        string tester (test);

        cout<<"Command is "<<tester<<endl;

        std::size_t pos = tester.find("-o");
        if (tester.find("-o")!=std::string::npos)
        {
         std::string str3 = tester.substr (pos+3,4);
         if(str3.find("PUT")!= std::string::npos)
         {
            put = true;
         }
         if(str3.find("GET")!= std::string::npos)
         {
            get = true;
         }
         if(str3.find("LIS")!= std::string::npos)
         {
            lis = true;
         }
         if(str3.find("CHK")!= std::string::npos)
         {
             chk = true;
         }
        }

        if(chk == true)
        {
            string username1;
            string password1;
            string linebuffer1;
            string confFile= "dfs3.conf";
            ifstream file1(confFile.c_str());
            while (file1 && getline(file1, linebuffer1))
            {
                if (linebuffer1.length() == 0)continue;
                if(linebuffer1.find("Password")!=std::string::npos)
                    {
                        password1 = linebuffer1.substr(10);
                    }
                if(linebuffer1.find("Username")!=std::string::npos)
                {
                    username1 = linebuffer1.substr(10);
                }
            }
            string authenticated = "FAILED";
            if(tester.find(username1)!=std::string::npos && tester.find(password1)!=std::string::npos)
                authenticated = "authenticated";
            write(connFd,authenticated.c_str(),strlen(authenticated.c_str()));
            write(connFd,"\r\n",2);
            chk = false;
        }
        if(tester.find(".txt")!=std::string::npos && put == true)
        {
            std::size_t pos = tester.find(".txt");
            string dirname = "";

            if(tester.find("/")!=std::string::npos)
            {
                size_t pos1 = tester.find("/");
                size_t startpos = tester.find("PUT");
                if(startpos < tester.size())
                {
                cout<<"CHECK"<<endl;
                dirname = tester.substr(startpos+5,pos1-8);
                }
                else
                {
                 dirname = tester.substr(2,pos1-2);
                 cout<<"Directory name when no puT "<<dirname<<endl;
                }
            }
            if(dirname.size()>1)
            {
                createDirectory("DataFiles/"+dirname);
            }
            filename_temp = "DataFiles/"+dirname+"/"+tester.substr(pos-5,11);
            string valuetoWrite = tester.substr(pos+8);

            if(tester.find("exit")==std::string::npos)
            {
            ofstream smallfile;
            smallfile.open(filename_temp.c_str(),ios::out | ios::binary);
            smallfile.write(valuetoWrite.c_str(),strlen(valuetoWrite.c_str()));
            smallfile.close();
            }
        }
      if(get == true && tester.find(".txt")!=std::string::npos)
        {
            std::size_t pos = tester.find(".txt");

            string dirname = "";

            if(tester.find("/")!=std::string::npos)
            {
                size_t pos1 = tester.find("/");
                dirname = tester.substr(0,pos1);
            }

            if(dirname.size()>1)
            filename_temp = "DataFiles/"+dirname+"/"+tester.substr(pos-5,11);
            else
            filename_temp = "DataFiles/"+tester.substr(pos-5,11);
            cout<<"Get this file "<<filename_temp<<endl;

            ifstream file(filename_temp.c_str());
            string linebuffer;
            string finalbuffer;
            bool file_exists = false;
            while (file && getline(file, linebuffer))
                {
                    file_exists = true;
                    if (linebuffer.length() == 0)continue;
                    finalbuffer+=linebuffer;
                    finalbuffer+="\n";
                }
            cout<<"File Content being written to the socket "<<finalbuffer<<endl;

            if(file_exists == true)
            {
            write(connFd,filename_temp.c_str(),strlen(filename_temp.c_str()));
            write(connFd,"\r\n",2);
            write(connFd,"filecontent",11);
            write(connFd,"\r\n",2);
            write(connFd,finalbuffer.c_str(),strlen(finalbuffer.c_str()));
            write(connFd,"\r\n",2);
            write(connFd,"exit",4);
            write(connFd,"\r\n",2);
            }
            else
            {
                cout<<"File is incomplete "<<endl;
                write(connFd,"incomplete",10);
                write(connFd,"\r\n",2);
                write(connFd,filename_temp.c_str(),strlen(filename_temp.c_str()));
                //write(connFd,"\r\n",2);
                write(connFd,"exit",4);
                write(connFd,"\r\n",2);
            }
        }
        if(lis == true)
        {
            string dir = "DataFiles";
            string dirname = "";

            if(tester.find("/")!=std::string::npos)
            {
                size_t pos1 = tester.find("/");
                dirname = tester.substr(2,pos1-2);
                dir = dir + "/"+dirname;
            }
            vector<string> files = vector<string>();

            getFilesfromDirectory(dir,files);

            for (int i = 0;i < files.size();i++)
            {
            write(connFd,files[i].c_str(),strlen(files[i].c_str()));
            write(connFd,"/",1);
            }
            lis = false;
            write(connFd,"exit",4);
            write(connFd,"\r\n",2);
        }
        if(tester.substr(0,4) == "exit")
            break;
        if(tester.substr(0,1) == " ")
            break;
        if(tester == "")
            break;
        }
    cout << "\nClosing thread and conn" << endl;
    close(connFd);
}
