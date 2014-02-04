////////////////////////////////////////////////////////////////////////
/// \file  GraphCluster.h
/// \brief
///
///
/// \author  andrzej.szelc@yale.edu
/// \author  ellen.klein@yale.edu
////////////////////////////////////////////////////////////////////////
#ifndef GRAPHCLUSTER_H
#define GRAPHCLUSTER_H

#include <vector>
#include <string>
#include "art/Framework/Core/ModuleMacros.h" 

#ifdef __CINT__
namespace art { 
  class EDProducer;
  class Event;
  class PtrVector;
  class Ptr;
  class ServiceHandle;
  class View;
}

namespace fhicl {
  class ParameterSet; 
}

namespace recob {
 class Hit; 
}


#else
#include "art/Framework/Core/EDProducer.h" 
#include "art/Persistency/Common/PtrVector.h"
#include "art/Persistency/Common/Ptr.h"
#include "art/Framework/Services/Registry/ActivityRegistry.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Principal/View.h"
#include "EventDisplay/InfoTransfer.h"
#include "Geometry/Geometry.h"

#endif


////////////////////////////////////////////////////////////////////////
//
// GraphCluster class
//
// andrzej.szelc@yale.edu
// ellen.klein@yale.edu
//
//  This dummy producer is designed to create a hitlist and maybe cluster from EVD input
////////////////////////////////////////////////////////////////////////

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
}
#include <sstream>
#include <math.h>
#include <algorithm>

// Framework includes
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 

#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h" 
#include "messagefacility/MessageLogger/MessageLogger.h" 

// LArSoft Includes

#include "RecoBase/Cluster.h"
#include "RecoBase/Hit.h"
#include "Utilities/AssociationUtil.h"
#include "Geometry/PlaneGeo.h"
#include "Utilities/GeometryUtilities.h"
#include "EventDisplay/GraphClusterAlg.h"

// ROOT 
#include "TMath.h"
#include "TF1.h"
#include "TH1D.h"



namespace util {
 class pxline;
 class pxpoint;
}

namespace geo {
  class Geometry;
}

namespace{
  void WriteMsg(const char* fcn)
  {
	mf::LogWarning("GraphCluster") << "GraphCluster::" << fcn << " \n";
  }
}

namespace evd {
  
  class InfoTransfer;
  
  class GraphCluster : public art::EDProducer {
    
  public:
    
    explicit GraphCluster(fhicl::ParameterSet const&); 
    virtual ~GraphCluster();
         
    void reconfigure(fhicl::ParameterSet const& p);
    void produce(art::Event& evt); 

  private:


    GraphClusterAlg fGClAlg;

  protected: 
    
  //  art::ServiceHandle<evd::InfoTransfer> intr;
  //  art::ServiceHandle<geo::Geometry>  geo;
    
    void GetStartEndHits(unsigned int plane, recob::Hit * starthit,recob::Hit * endhit);
    void GetStartEndHits(unsigned int plane);
    
    //void GetHitList(unsigned int plane,std::vector< art::Ptr <recob::Hit> > ptrhitlist);
    void GetHitList(unsigned int plane, art::PtrVector <recob::Hit>  &ptrhitlist);
    
    
    std::vector < util::pxline > GetSeedLines();
    
  //  int GetMetaInfo(art::Event& evt);
    
    unsigned int fNPlanes;
    
    int TestFlag;
    int fRun;
    int fSubRun;
    int fEvent;
    
    
   
    
    std::vector< recob::Hit * > starthit;
    std::vector< recob::Hit * > endhit;
    
//     std::vector < std::vector< recob::Hit * > > hitlist;
    
    std::vector < util::pxline > startendpoints;
    
//     std::vector <unsigned int> swire;
//     std::vector <unsigned int> ewire;
//     std::vector <double> stime;
//     std::vector <double> etime;
    
    
    
  }; // class GraphCluster
  
  

  //-------------------------------------------------
  GraphCluster::GraphCluster(fhicl::ParameterSet const& pset) : 
  fGClAlg(pset.get< fhicl::ParameterSet >("GraphClusterAlg"))
  {
    this->reconfigure(pset);
    art::ServiceHandle<geo::Geometry>  geo;
	
	
    produces< std::vector<recob::Cluster>                >();
    produces< art::Assns<recob::Cluster, recob::Hit>     >();
    produces< std::vector < art::PtrVector <recob::Cluster> >  >();
	
	
    fNPlanes = geo->Nplanes();
    starthit.resize(fNPlanes);
    endhit.resize(fNPlanes);
    
    
    startendpoints.resize(fNPlanes);
//     swire.resize(fNPlanes);
//     ewire.resize(fNPlanes);
//     stime.resize(fNPlanes);
//     etime.resize(fNPlanes);
  }
  
  
  //-------------------------------------------------
  GraphCluster::~GraphCluster()
  {
  }
  
  //-------------------------------------------------
  void GraphCluster::reconfigure(fhicl::ParameterSet const& /*pset*/)
  {


    return;
  }
  
  //
  //-------------------------------------------------
  /// \todo This method appears to produce a recob::Cluster really as it is 
  /// \todo a collection of 2D clusters from single planes
  void GraphCluster::produce(art::Event& evt)
  { 
        
    std::unique_ptr<std::vector<recob::Cluster> > Graphcol(new std::vector<recob::Cluster>);
    std::unique_ptr< art::Assns<recob::Cluster, recob::Hit>     > hassn(new art::Assns<recob::Cluster, recob::Hit>);
    // 	std::unique_ptr< art::Assns<recob::Cluster, recob::Cluster>     > classn(new art::Assns<recob::Cluster, recob::Cluster>);
    std::unique_ptr< std::vector < art::PtrVector < recob::Cluster > > > classn(new std::vector < art::PtrVector < recob::Cluster > >);

	
	
	 
    art::ServiceHandle<geo::Geometry>  geo;

    // check if evt and run numbers check out, etc...
    if(fGClAlg.CheckValidity(  evt ) == -1)
      {
	return; 
      }
	
	
    for(unsigned int ip=0;ip<fNPlanes;ip++) {
      startendpoints[ip]=util::pxline();  //assign empty pxline
      
    }
	
    std::vector < art::PtrVector < recob::Hit > > hitlist;
    hitlist.resize(fNPlanes);
	 
    for(unsigned int ip=0;ip<fNPlanes;ip++) {
		
	fGClAlg.GetHitListAndEndPoints(ip,hitlist[ip],startendpoints[ip]);
      // Read in the Hit List object(s).
     //fGClAlg.GetHitList(ip,hitlist[ip]); 
	   
      if(hitlist[ip].size()==0)
	continue;
      //Read in the starthit:
      // GetStartEndHits(ip, starthit[ip],endhit[ip]);	
	
          
	      
	      
      //fGClAlg.GetStartEndHits(&startendpoints[ip]);
	   
      if(hitlist[ip].size()>0 && !(TestFlag==-1 ) )   //old event or transfer not ready
	{
	  double swterror=0.,ewterror=0.;
		  
	  if(startendpoints[ip].w0==0 )
	    swterror=999;
		  
	  if(startendpoints[ip].t1==0 )
	    ewterror=999;
		  
	  std::cout << " clustering @ " <<startendpoints[ip].w0 << " +/- "<< swterror
		    <<" " <<  startendpoints[ip].t0<< " +/- "<< swterror
		    <<" " <<  startendpoints[ip].w1<< " +/- "<< ewterror
		    <<" " << startendpoints[ip].t1<< " +/- "<< ewterror << std::endl;  
		  
	  recob::Cluster temp(startendpoints[ip].w0, swterror,
			      startendpoints[ip].t0, swterror,
			      startendpoints[ip].w1, ewterror,
			      startendpoints[ip].t1, ewterror,  
			      0, 0, 0,0,5.,
			      geo->Plane(ip,0,0).View(),
			      ip);
  
	  Graphcol->push_back(temp);
	  // associate the hits to this cluster
	  util::CreateAssn(*this, evt, *Graphcol, hitlist[ip], *hassn);
	}

    }// end of loop on planes
   
    art::PtrVector < recob::Cluster > cvec;
	
    for(unsigned int ip=0;ip<fNPlanes;ip++)  {
      art::ProductID aid = this->getProductID< std::vector < recob::Cluster > >(evt);
      art::Ptr< recob::Cluster > aptr(aid, ip, evt.productGetter(aid));
      cvec.push_back(aptr);
    }
   
    classn->push_back(cvec);
   
    // 	for(unsigned int ip=0;ip<fNPlanes;ip++) {
    // 	  for(unsigned int jp=ip+1;jp<fNPlanes;jp++) {
    // 	    util::CreateSameAssn(*this, evt, *Graphcol, *Graphcol, *classn,ip,ip+1,jp );
    // 	  //  std::cout << "associating cluster" << ip <<" with cluster " << jp << std::endl;
    // 	  }
    // 	}
    //     
    
    evt.put(std::move(Graphcol));
    evt.put(std::move(hassn));
    evt.put(std::move(classn));
	
    return;
  } // end of produce



  DEFINE_ART_MODULE(GraphCluster)
  
} //end of evd namespace


#endif // GRAPHCLUSTER_H
