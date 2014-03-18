#include "Validation/MuonGEMHits/interface/GEMSimTrackMatch.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/GEMDigi/interface/GEMDigiCollection.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include <TMath.h>
#include <TH1F.h>

GEMTrackMatch::GEMTrackMatch(DQMStore* dbe, std::string simInputLabel , edm::ParameterSet cfg)
{
   cfg_= cfg; 
   simInputLabel_= simInputLabel;
   dbe_= dbe;
}


GEMTrackMatch::~GEMTrackMatch() {
}

bool GEMTrackMatch::isSimTrackGood(const SimTrack &t)
{
  // SimTrack selection
  if (t.noVertex())   return false; 
  if (t.noGenpart()) return false;
  if (std::abs(t.type()) != 13) return false; // only interested in direct muon simtracks
  if (t.momentum().pt() < minPt_ ) return false;
  const float eta(std::abs(t.momentum().eta()));
  if (eta > maxEta_ || eta < minEta_ ) return false; // no GEMs could be in such eta
  return true;
}


void GEMTrackMatch::buildLUT()
{
  const int maxChamberId_ = theGEMGeometry->regions()[0]->stations()[0]->superChambers().size();
  edm::LogInfo("GEMTrackMatch")<<"max chamber "<<maxChamberId_<<"\n";
  std::vector<int> pos_ids;
  pos_ids.push_back(GEMDetId(1,1,1,1,maxChamberId_,1).rawId());

  std::vector<int> neg_ids;
  neg_ids.push_back(GEMDetId(-1,1,1,1,maxChamberId_,1).rawId());

  // VK: I would really suggest getting phis from GEMGeometry
  
  std::vector<float> phis;
  phis.push_back(0.);
  for(int i=1; i<maxChamberId_+1; ++i)
  {
    pos_ids.push_back(GEMDetId(1,1,1,1,i,1).rawId());
    neg_ids.push_back(GEMDetId(-1,1,1,1,i,1).rawId());
    phis.push_back(i*10.);
  }
  positiveLUT_ = std::make_pair(phis,pos_ids);
  negativeLUT_ = std::make_pair(phis,neg_ids);
}


void GEMTrackMatch::setGeometry(const GEMGeometry* geom)
{
  theGEMGeometry = geom;
  int  odd_roll = 1;
  int even_roll = 1;
  if (  theGEMGeometry->idToDetUnit( GEMDetId(1,1,1,1,1,odd_roll) ) == nullptr) { 
     LogDebug("GEMTrackMatch")<<" +++ error : Can not find odd chamber's roll 1\n";
     odd_roll = 2; 
  }
  if ( theGEMGeometry->idToDetUnit( GEMDetId(1,1,1,1,2,even_roll) ) == nullptr) {
     LogDebug("GEMTrackMatch")<<" +++ error : Can not find even chamber's roll 1\n";
     even_roll = 2;
  }
  //odd part
  const auto top_chamber_odd = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,1,odd_roll)));  // chamber 1, roll : odd_roll 
  const int nEtaPartitions_odd(theGEMGeometry->chamber(GEMDetId(1,1,1,1,1,odd_roll))->nEtaPartitions());
  const auto bottom_chamber_odd = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,1,nEtaPartitions_odd)));
  float top_half_striplength = top_chamber_odd->specs()->specificTopology().stripLength()/2.;
  float bottom_half_striplength = bottom_chamber_odd->specs()->specificTopology().stripLength()/2.;
  LocalPoint lp_top(0., top_half_striplength, 0.);
  LocalPoint lp_bottom(0., -bottom_half_striplength, 0.);
  GlobalPoint gp_top = top_chamber_odd->toGlobal(lp_top);
  GlobalPoint gp_bottom = bottom_chamber_odd->toGlobal(lp_bottom);

  radiusCenter_odd_ = (gp_bottom.perp() + gp_top.perp())/2.;
  chamberHeight_odd_ = gp_top.perp() - gp_bottom.perp();


  //even part
  const auto top_chamber_even = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,2,even_roll)));  // chamber 2, roll : even_roll
  const int nEtaPartitions_even(theGEMGeometry->chamber( GEMDetId(1,1,1,1,2,even_roll) )->nEtaPartitions());
  const auto bottom_chamber_even = static_cast<const GEMEtaPartition*>(theGEMGeometry->idToDetUnit(GEMDetId(1,1,1,1,2,nEtaPartitions_even)));
  top_half_striplength = top_chamber_even->specs()->specificTopology().stripLength()/2.;
  bottom_half_striplength = bottom_chamber_even->specs()->specificTopology().stripLength()/2.;
  lp_top = LocalPoint(0., top_half_striplength, 0.);
  lp_bottom = LocalPoint(0., -bottom_half_striplength, 0.);
  gp_top = top_chamber_even->toGlobal(lp_top);
  gp_bottom = bottom_chamber_even->toGlobal(lp_bottom);

  radiusCenter_even_ = (gp_bottom.perp() + gp_top.perp())/2.;
  chamberHeight_even_ = gp_top.perp() - gp_bottom.perp();

  buildLUT();
}  


std::pair<int,int> GEMTrackMatch::getClosestChambers(int region, float phi)
{
  const int maxChamberId_ = theGEMGeometry->regions()[0]->stations()[0]->superChambers().size();
  auto& phis(positiveLUT_.first);
  auto upper = std::upper_bound(phis.begin(), phis.end(), phi);
  auto& LUT = (region == 1 ? positiveLUT_.second : negativeLUT_.second);
  return std::make_pair(LUT.at(upper - phis.begin()), (LUT.at((upper - phis.begin() + 1)%maxChamberId_)));
}


