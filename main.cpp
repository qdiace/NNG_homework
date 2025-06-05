#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sqlite3.h> 
#include <map>
#include <math.h>

using namespace std;

struct pointnnum
{
    int num; double lat; double lon;
};
double dist(double x1,double y1,double x2,double y2)
{
    // 1 lat deg = 111 km; around N47, 1 lon deg = ca. 75km
    return sqrt(5625*(x1-x2)*(x1-x2)+12321*(y1-y2)*(y1-y2));
}

class points{
    public:
        void instnew(int num, double lat, double lon)
        {
            struct pointnnum help;
            nop++;
            help.num=num;help.lat=lat;help.lon=lon;
            content.insert({nop, help});
            // Normally, nop == num; to be sure, keep this structure.
        };
        int findclosest(double lat, double lon)
        {
            double mindist=40000000;
            double newdist;
            int minind=0;
            for (auto i : content)
            {
                // TODO: an optimal search, e.g., Voronoi diagram
                newdist=dist(i.second.lon,i.second.lat,lon,lat);
                if (newdist<mindist)
                {
                    mindist=newdist;
                    minind=i.second.num;
                }
            }
            return minind;
        }
        void enume()
        {
            // Served debug aspects, can be removed.
            for (auto i:content)
            {
                cout<<i.first<<" "<<i.second.num<<" "<<i.second.lat<<" "<<i.second.lon<<" "<<endl;
            }
        }
        points()
        {
            nop=0;
            content.clear();
        };
        ~points()
        {
            nop=0;
            content.clear();
        };
    private:
        int nop;
        map <int, struct pointnnum> content;
    
};

int main(int argc, char* argv[]) {
   sqlite3 *db;
   sqlite3_stmt* stmt;
   char *zErrMsg = 0;
   ifstream ifile;
   int rc,rrid,closest,helpnum;
   bool flag,done;
    string query, line, filename;
    points allpts;
    double prevlat, prevlon,prevtime,width,helplat, helplon;
   rc = sqlite3_open("road.db", &db);

   if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return(0);
   } else {
      fprintf(stderr, "Opened database successfully\n");
   }

    query = "SELECT ROAD_PNT_ID, LAT, LON FROM ROAD_POINTS;";
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement\n";
        sqlite3_close(db);
        return 1;    
    }
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        helpnum = sqlite3_column_double(stmt, 0);
        helplat = sqlite3_column_double(stmt, 1);
        helplon = sqlite3_column_double(stmt, 2);
        allpts.instnew(helpnum, helplat, helplon);
    }
   filename="track_"+string(argv[1])+".csv";
    ifile.open(filename);
    getline(ifile, line); // Skipping header from CSV file
    int n=0;
    flag=false;
    while (getline(ifile, line))
    {
        n++;
        istringstream ss(line);
        string token,tokeni,tokenf,tokenla,tokenlo,tokend;
        getline(ss, token, ',');
        istringstream sss(token);
        getline (sss, tokeni , '.');
        getline (sss, tokenf , '.');
        
        getline(ss, tokenla, ',');
        getline(ss, tokenlo, ',');
        getline(ss, tokend, ',');
        query = "insert into GPS_TRACK_POINTS values ("+to_string(n)+",NULL, "+tokeni+","+tokend+","+tokenla+","+tokenlo+",NULL,"+tokenf[0]+")";
        if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK)
        {
            std::cerr << "Failed to insert\n";
            sqlite3_close(db);
            return 1;    
        }
        done=false;
        closest=allpts.findclosest(stod(tokenla),stod(tokenlo));
        query="select ROAD_WIDTH_IN_METER, LAT, LON from ROADS inner join ROAD_POINTS on REF_ROAD_ID=ROAD_ID WHERE ROAD_PNT_ID="+to_string(closest);
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to select from join\n";
            sqlite3_close(db);
            return 1;    
        }
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            width =   sqlite3_column_int(stmt, 0);
            helplat = sqlite3_column_double(stmt, 1);
            helplon = sqlite3_column_double(stmt, 2);
        }
        cout<<"Width="<<width<<" "<<1000*dist(helplon,helplat,stod(tokenlo),stod(tokenla))<<endl;
        
        if(1000*dist(helplon,helplat,stod(tokenlo),stod(tokenla))>6*width)
        {
            // Too far even from the closest point
            done=true;
            query="update GPS_TRACK_POINTS set MATCHING_ERR_CODE=2 where GPS_TRK_ID="+to_string(n);
            if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK)
            {
               std::cerr << "Failed to update\n";
               sqlite3_close(db);
               return 1;    
            }                                       
        }
        
        
        if ((n>1) && (!done))
        {
            if (dist(stod(tokenlo), stod (tokenla), prevlon, prevlat)/(stod(token)-prevtime)>120)
            {
                // If a car is driving with over 120 m/s, it must be measure error.
                if (!flag)
                {
                    flag=true;
                    query="update GPS_TRACK_POINTS set MATCHING_ERR_CODE=1 where GPS_TRK_ID="+to_string(n);
                    if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK)
                    {
                        std::cerr << "Failed to update II\n";
                        sqlite3_close(db);
                        return 1;    
                    }                    
                }
                else
                {
                    query="update GPS_TRACK_POINTS set REF_ROAD_ID=A.REF_ROAD_ID from (select REF_ROAD_ID from ROAD_POINTS  WHERE ROAD_PNT_ID="+
                    to_string(closest)+") as A where GPS_TRK_ID="+to_string(n);
                    if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK)
                    {
                        std::cerr << "Failed to update III\n";
                        sqlite3_close(db);
                        return 1;    
                    }                    
                    flag=false;
                }
            }
            else
            {
                    query="update GPS_TRACK_POINTS set REF_ROAD_ID=A.REF_ROAD_ID from (select REF_ROAD_ID from ROAD_POINTS  WHERE ROAD_PNT_ID="+
                    to_string(closest)+") as A where GPS_TRK_ID="+to_string(n);
                    cout<<"Update IV "<<query<<endl;
                    if (sqlite3_exec(db, query.c_str(), 0, 0, 0) != SQLITE_OK)
                    {
                        std::cerr << "Failed to update IV\n";
                        sqlite3_close(db);
                        return 1;    
                    }                    
                
            }
        
        }
        prevlat=stod(tokenla);prevlon=stod(tokenlo);prevtime=stod(token);
    }
    ifile.close();
   sqlite3_close(db);
}
