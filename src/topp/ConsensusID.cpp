// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2015.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Sven Nahnsen, Hendrik Weisser $
// --------------------------------------------------------------------------

#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithm.h>
#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithmPEPMatrix.h>
#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithmPEPIons.h>
#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithmBest.h>
#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithmAverage.h>
#include <OpenMS/ANALYSIS/ID/ConsensusIDAlgorithmRanks.h>
#include <OpenMS/ANALYSIS/MAPMATCHING/FeatureGroupingAlgorithmQT.h>
#include <OpenMS/APPLICATIONS/TOPPBase.h>
#include <OpenMS/CONCEPT/VersionInfo.h>
#include <OpenMS/FORMAT/IdXMLFile.h>
#include <OpenMS/FORMAT/FeatureXMLFile.h>
#include <OpenMS/FORMAT/ConsensusXMLFile.h>
#include <OpenMS/FORMAT/FileHandler.h>
#include <OpenMS/FORMAT/FileTypes.h>

using namespace OpenMS;
using namespace std;

//-------------------------------------------------------------
// Doxygen docu
//-------------------------------------------------------------

/**
    @page TOPP_ConsensusID ConsensusID

    @brief Computes a consensus identification from peptide identification engines.

    <CENTER>
    <table>
        <tr>
            <td ALIGN = "center" BGCOLOR="#EBEBEB"> potential predecessor tools </td>
            <td VALIGN="middle" ROWSPAN=4> \f$ \longrightarrow \f$ ConsensusID \f$ \longrightarrow \f$</td>
            <td ALIGN = "center" BGCOLOR="#EBEBEB"> potential successor tools </td>
        </tr>
        <tr>
            <td VALIGN="middle" ALIGN = "center" ROWSPAN=1> @ref TOPP_IDPosteriorErrorProbability </td>
            <td VALIGN="middle" ALIGN = "center" ROWSPAN=3> @ref TOPP_PeptideIndexer </td>
        </tr>
        <tr>
          <td VALIGN="middle" ALIGN="center" ROWSPAN=1> @ref TOPP_IDFilter </td>
        </tr>
        <tr>
          <td VALIGN="middle" ALIGN="center" ROWSPAN=1> @ref TOPP_IDMapper </td>
        </tr>
    </table>
    </CENTER>

    This implementation (for PEPMatrix and PEPIons) is described in
    <p>
    Nahnsen S, Bertsch A, Rahnenfuehrer J, Nordheim A, Kohlbacher O<br>
    Probabilistic Consensus Scoring Improves Tandem Mass Spectrometry Peptide Identification<br>
    Journal of Proteome Research (2011), DOI: 10.1021/pr2002879<br>
    </p>

    The input file can contain several searches, e.g., from several identification engines. In order
    to use the PEPMatrix or the PEPIons algorithm, posterior
    error probabilities (PEPs) need to be calculated using the @ref TOPP_IDPosteriorErrorProbability tool
    for all individual search engines. After PEP calculation, the different search engine results
    have to be combined using @ref TOPP_IDMerger. Identification runs can be mapped
    to featureXML and consensusXML with the @ref TOPP_IDMapper tool. The merged file can now be fed into
    into the @ref TOPP_ConsensusID tool. For the statistical assessment of the results it is recommended
    to use target-decoy databases for peptide identifications. The false discovery rates (FDRs) can be
    calculated using the @ref TOPP_FalseDiscoveryRate tool.

    @note Currently mzIdentML (mzid) is not directly supported as an input/output format of this tool. Convert mzid files to/from idXML using @ref TOPP_IDFileConverter if necessary.

    <B>The command line parameters of this tool are:</B>
    @verbinclude TOPP_ConsensusID.cli
    <B>INI file documentation of this tool:</B>
    @htmlinclude TOPP_ConsensusID.html

    For the parameters of the algorithm section see the algorithms documentation: @n
    @ref OpenMS::ConsensusID "Consensus algorithm" @n
*/

// We do not want this class to show up in the docu:
/// @cond TOPPCLASSES

class TOPPConsensusID :
  public TOPPBase
{
public:
  TOPPConsensusID() :
    TOPPBase("ConsensusID", "Computes a consensus of peptide identifications of several identification engines.")
  {
  }

protected:

  Param getSubsectionDefaults_(const String& section) const
  {
    Param algo_param;
    if (section == "PEPMatrix")
    {
      algo_param = ConsensusIDAlgorithmPEPMatrix().getDefaults();
    }
    else // section == "PEPIons"
    {
      algo_param = ConsensusIDAlgorithmPEPIons().getDefaults();
    }
    algo_param.remove("considered_hits"); // defined in the base class
    return algo_param;
  }

  void registerOptionsAndFlags_()
  {
    registerInputFile_("in", "<file>", "", "input file");
    setValidFormats_("in", ListUtils::create<String>("idXML,featureXML,consensusXML"));
    registerOutputFile_("out", "<file>", "", "output file");
    setValidFormats_("out", ListUtils::create<String>("idXML,featureXML,consensusXML"));

    addEmptyLine_();
    registerDoubleOption_("rt_delta", "<value>", 0.1, "Maximum allowed precursor RT deviation between identifications belonging to the same spectrum.", false);
    setMinFloat_("rt_delta", 0.0);
    registerDoubleOption_("mz_delta", "<value>", 0.1, "Maximum allowed precursor m/z deviation between identifications belonging to the same spectrum.", false);
    setMinFloat_("mz_delta", 0.0);
    registerIntOption_("considered_hits", "<number>", 10, "The number of top hits that are used for the consensus scoring ('0' for all hits).", false);
    setMinInt_("considered_hits", 0);

    registerStringOption_("algorithm", "<choice>", "PEPMatrix",
                          "Algorithm used for consensus scoring.\n"
                          "* PEPMatrix: scoring based on posterior error probabilities (PEPs) and peptide sequence similarities. This algorithm uses a substitution matrix to score the similarity of sequences not listed by all search engines. Make sure that the scores for all peptide IDs are PEPs!\n"
                          "* PEPIons: scoring based on posterior error probabilities (PEPs) and fragment ion similarities. Make sure that the scores for all peptide IDs are PEPs!\n"
                          "* best: uses the best score of any search engine as the consensus score of each peptide ID. Make sure that all peptide IDs use the same score type!\n"
                          "* average: uses the average score of all search engines as the consensus score of each peptide ID. Make sure that all peptide IDs use the same score type!\n"
                          "* ranks: calculates a consensus score based on the ranks of peptide IDs in results of the different search engines. The final score is in the range (0, 1], with 1 being the best score. The input peptide IDs do not need to have the same score type.", false);
    setValidStrings_("algorithm", ListUtils::create<String>("PEPMatrix,PEPIons,best,average,ranks"));

    // subsections appear in alphabetical (?) order, independent of the order
    // in which they were registered:
    registerSubsection_("PEPIons", "PEPIons algorithm parameters");
    registerSubsection_("PEPMatrix", "PEPMatrix algorithm parameters");
  }

  void setProteinIdentifications_(vector<ProteinIdentification>& prot_ids)
  {
    prot_ids.clear();
    prot_ids.resize(1);
    prot_ids[0].setDateTime(DateTime::now());
    prot_ids[0].setSearchEngine("OpenMS/ConsensusID");
    prot_ids[0].setSearchEngineVersion(VersionInfo::getVersion());
  }

  ExitCodes main_(int, const char**)
  {
    String in = getStringOption_("in");
    FileTypes::Type in_type = FileHandler::getType(in);
    String out = getStringOption_("out");
    double rt_delta = getDoubleOption_("rt_delta");
    double mz_delta = getDoubleOption_("mz_delta");

    //----------------------------------------------------------------
    // set up ConsensusID
    //----------------------------------------------------------------
    ConsensusIDAlgorithm* consensus;
    Param algo_param;
    String algorithm = getStringOption_("algorithm");
    if (algorithm == "PEPMatrix")
    {
      consensus = new ConsensusIDAlgorithmPEPMatrix();
      algo_param = getParam_().copy("PEPMatrix:", true);
    }
    else if (algorithm == "PEPIons")
    {
      consensus = new ConsensusIDAlgorithmPEPIons();
      algo_param = getParam_().copy("PEPIons:", true);
    }
    else if (algorithm == "best")
    {
      consensus = new ConsensusIDAlgorithmBest();
    }
    else if (algorithm == "average")
    {
      consensus = new ConsensusIDAlgorithmAverage();
    }
    else // algorithm == "ranks"
    {
      consensus = new ConsensusIDAlgorithmRanks();
    }
    algo_param.setValue("considered_hits", getIntOption_("considered_hits"));

    //----------------------------------------------------------------
    // idXML
    //----------------------------------------------------------------
    if (in_type == FileTypes::IDXML)
    {
      vector<ProteinIdentification> prot_ids;
      vector<PeptideIdentification> pep_ids;
      String document_id;
      IdXMLFile().load(in, prot_ids, pep_ids, document_id);

      // merge peptide IDs by precursor position - this is equivalent to a
      // feature linking problem (peptide IDs from different ID runs <-> 
      // features from different maps), so we bring the data into a format
      // suitable for a feature grouping algorithm:
      vector<FeatureMap> maps(prot_ids.size());
      map<String, Size> id_mapping; // mapping: run ID -> index (of feature map)
      for (Size i = 0; i < prot_ids.size(); ++i)
      {
        id_mapping[prot_ids[i].getIdentifier()] = i;
      }

      for (vector<PeptideIdentification>::iterator pep_it = pep_ids.begin();
           pep_it != pep_ids.end(); ++pep_it)
      {
        String run_id = pep_it->getIdentifier();
        if (!pep_it->hasRT() || !pep_it->hasMZ())
        {
          LOG_FATAL_ERROR << "Peptide ID without RT and/or m/z information found in identification run '" + run_id + "'.\nMake sure that this information is included for all IDs when generating/converting search results. Aborting!" << endl;
          return INCOMPATIBLE_INPUT_DATA;
        }

        Feature feature;
        feature.setRT(pep_it->getRT());
        feature.setMZ(pep_it->getMZ());
        feature.getPeptideIdentifications().push_back(*pep_it);
        maps[id_mapping[run_id]].push_back(feature);
      }
      // precondition for "FeatureGroupingAlgorithmQT::group":
      for (vector<FeatureMap>::iterator map_it = maps.begin();
           map_it != maps.end(); ++map_it)
      {
        map_it->updateRanges();
      }

      FeatureGroupingAlgorithmQT linker;
      Param linker_params = linker.getDefaults();
      linker_params.setValue("use_identifications", "false");
      linker_params.setValue("ignore_charge", "true");
      linker_params.setValue("distance_RT:max_difference", rt_delta);
      linker_params.setValue("distance_MZ:max_difference", mz_delta);
      linker_params.setValue("distance_MZ:unit", "Da");
      linker.setParameters(linker_params);

      ConsensusMap grouping;
      linker.group(maps, grouping);

      // compute consensus
      algo_param.setValue("number_of_runs", prot_ids.size());
      consensus->setParameters(algo_param);
      pep_ids.clear();
      for (ConsensusMap::Iterator it = grouping.begin(); it != grouping.end();
           ++it)
      {
        consensus->apply(it->getPeptideIdentifications());
        if (!it->getPeptideIdentifications().empty())
        {
          PeptideIdentification& pep_id = it->getPeptideIdentifications()[0];
          pep_id.setRT(it->getRT());
          pep_id.setMZ(it->getMZ());
          pep_ids.push_back(pep_id);
        }
      }

      // create new identification run
      setProteinIdentifications_(prot_ids);

      // store consensus
      IdXMLFile().store(out, prot_ids, pep_ids);
   }

    //----------------------------------------------------------------
    // featureXML
    //----------------------------------------------------------------
    if (in_type == FileTypes::FEATUREXML)
    {
      //load map
      FeatureMap map;
      FeatureXMLFile().load(in, map);

      //compute consensus
      algo_param.setValue("ranks:number_of_runs",
                          map.getProteinIdentifications().size());
      consensus->setParameters(algo_param);
      for (FeatureMap::Iterator it = map.begin(); it != map.end(); ++it)
      {
        consensus->apply(it->getPeptideIdentifications());
      }

      //create new identification run
      setProteinIdentifications_(map.getProteinIdentifications());

      //store consensus
      FeatureXMLFile().store(out, map);
    }

    //----------------------------------------------------------------
    // consensusXML
    //----------------------------------------------------------------
    if (in_type == FileTypes::CONSENSUSXML)
    {
      //load map
      ConsensusMap map;
      ConsensusXMLFile().load(in, map);

      //compute consensus
      algo_param.setValue("ranks:number_of_runs",
                          map.getProteinIdentifications().size());
      consensus->setParameters(algo_param);
      for (ConsensusMap::Iterator it = map.begin(); it != map.end(); ++it)
      {
        consensus->apply(it->getPeptideIdentifications());
      }

      //create new identification run
      setProteinIdentifications_(map.getProteinIdentifications());

      //store consensus
      ConsensusXMLFile().store(out, map);
    }

    delete consensus;

    return EXECUTION_OK;
  }

};


int main(int argc, const char** argv)
{
  TOPPConsensusID tool;
  return tool.main(argc, argv);
}

/// @endcond
