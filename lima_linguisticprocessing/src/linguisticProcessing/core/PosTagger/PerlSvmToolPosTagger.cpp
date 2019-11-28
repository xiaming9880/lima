/*
    Copyright 2002-2013 CEA LIST

    This file is part of LIMA.

    LIMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LIMA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with LIMA.  If not, see <http://www.gnu.org/licenses/>
*/

#include "PerlSvmToolPosTagger.h"

#include "linguisticProcessing/core/LinguisticResources/LinguisticResources.h"
#include "linguisticProcessing/common/PropertyCode/PropertyCodeManager.h"
#include "common/XMLConfigurationFiles/xmlConfigurationFileExceptions.h"
#include "common/MediaticData/mediaticData.h"
#include "common/tools/FileUtils.h"
#include "common/Data/strwstrtools.h"
#include "common/time/timeUtilsController.h"

#include <boost/algorithm/string.hpp>

#include <QProcess>
#include <QTemporaryFile>

using namespace Lima::Common::AnnotationGraphs;
using namespace Lima::Common::Misc;
using namespace Lima::Common::MediaticData;
using namespace Lima::Common::PropertyCode;
using namespace Lima::Common::XMLConfigurationFiles;
using namespace Lima::LinguisticProcessing::LinguisticAnalysisStructure;

namespace Lima
{
namespace LinguisticProcessing
{
namespace PosTagger
{

PerlSvmToolPosTaggerFactory* PerlSvmToolPosTaggerFactory::s_instance=new PerlSvmToolPosTaggerFactory(PERLSVMTOOLPOSTAGGER_CLASSID);

PerlSvmToolPosTaggerFactory::PerlSvmToolPosTaggerFactory(const std::string& id) :
    InitializableObjectFactory<MediaProcessUnit>(id)
{}

MediaProcessUnit* PerlSvmToolPosTaggerFactory::create(
  Common::XMLConfigurationFiles::GroupConfigurationStructure& unitConfiguration,
  MediaProcessUnit::Manager* manager) const
{
//   PTLOGINIT;
  MediaProcessUnit* posTagger = new PerlSvmToolPosTagger;
  posTagger->init(unitConfiguration,manager);

  return posTagger;
}

  struct PredData
  {
    PredData() :
        m_predMicro(),
        m_predIndex(),
    m_predPredMicros() {}
    LinguisticCode m_predMicro;
    uint64_t m_predIndex;
    std::vector<LinguisticCode> m_predPredMicros;
    inline bool operator<(const PredData& pd) const { return m_predMicro<pd.m_predMicro; }
  };

  typedef std::map< LinguisticCode, std::vector<PredData> > MicroCatDataMap;
  typedef std::map< LinguisticCode, std::vector<PredData> >::iterator MicroCatDataMapItr;
  typedef std::vector<PredData>::iterator PredDataVectorItr;
  typedef std::vector<PredData>::const_iterator PredDataVectorCItr;

  struct StepData
  {
    LinguisticGraphVertex m_srcVertex;
    std::vector<uint64_t> m_predStepIndexes;
    MicroCatDataMap m_microCatsData;
  };

  typedef std::vector<StepData> StepDataVector;
  typedef std::vector<StepData>::iterator StepDataVectorItr;

  struct TargetVertexId
  {
    LinguisticGraphVertex m_sourceVx;
    LinguisticCode m_categ;
    std::vector<LinguisticCode> m_preds;
    bool operator<(const TargetVertexId& tvi) const
    {
      if (m_sourceVx!=tvi.m_sourceVx) return m_sourceVx<tvi.m_sourceVx;
      if (m_categ!=tvi.m_categ) return m_categ<tvi.m_categ;
      return m_preds<tvi.m_preds;
    }
  };

class PerlSvmToolPosTaggerPrivate
{
  friend class PerlSvmToolPosTagger;

  PerlSvmToolPosTaggerPrivate() :  m_microAccessor(nullptr) {}
  ~PerlSvmToolPosTaggerPrivate() {}

  QString m_svmtool;
  const Common::PropertyCode::PropertyAccessor* m_microAccessor;
  MediaId m_language;
  std::string m_model;
  bool m_allFeatures;
  QStringList m_features;
};

PerlSvmToolPosTagger::PerlSvmToolPosTagger() :
  m_d(new PerlSvmToolPosTaggerPrivate())
{
}

PerlSvmToolPosTagger::~PerlSvmToolPosTagger()
{
  delete m_d;
}


void PerlSvmToolPosTagger::init(
    GroupConfigurationStructure& unitConfiguration,
    Manager* manager)

{
  /** @addtogroup ProcessUnitConfiguration
  * - <b>&lt;group name="..." class="PerlSvmToolPosTagger"&gt;</b>
  *    -  defaultCategory : micro category to use when no categories are
  *         available. For example, used before and after text to
  *         disambiguate.
  *  -  stopCategories : list of categories that delimits independant
  *         segment to disambiguate.
  *    -  trigramFile : file containing the trigram matrix
  *    -  bigramFile : file containing the bigram matrix
  */

  PTLOGINIT;
  m_d->m_language = manager->getInitializationParameters().media;
  const auto& ldata = static_cast<const LanguageData&>(
      Common::MediaticData::MediaticData::single().mediaData(m_d->m_language));
  const auto& microManager = ldata.getPropertyCodeManager().getPropertyManager("MICRO");
  m_d->m_microAccessor = &(microManager.getPropertyAccessor());
  auto resourcesPath = MediaticData::single().getResourcesPath();
  try
  {
    m_d->m_svmtool = QString::fromUtf8(unitConfiguration.getParamsValueAtKey("program").c_str());
  }
  catch (Common::XMLConfigurationFiles::NoSuchParam& )
  {
    LWARN << "No SVMTool program defined in configuration file !";
    throw InvalidConfiguration();
  }
  try
  {
    auto modelName = unitConfiguration.getParamsValueAtKey("model");
    // add .DICT to find the file, remove it to get the generic model name + path
    m_d->m_model = findFileInPaths(resourcesPath.c_str(),
                                   modelName.append(".DICT").c_str()).toUtf8().constData();
    boost::replace_last(m_d->m_model,".DICT","");
  }
  catch (Common::XMLConfigurationFiles::NoSuchParam& )
  {
    LWARN << "No SVMTool model defined in configuration file !";
    throw InvalidConfiguration();
  }
  try
  {
    m_d->m_allFeatures = unitConfiguration.getBooleanParameter("allFeatures");
  }
  catch (Common::XMLConfigurationFiles::NoSuchParam& )
  {
    // Ignored parameters allFeatures and features are optional. Then use only
    // main tag (micro category)
  }
  if (!m_d->m_allFeatures)
  {
    try
    {
      auto features = unitConfiguration.getListsValueAtKey("features");
      for (const auto& feature: features)
      {
        m_d->m_features << QString::fromUtf8(feature.c_str());
      }
    }
    catch (Common::XMLConfigurationFiles::NoSuchParam& )
    {
      // Ignored parameters allFeatures and features are optional. Then use only
      // main tag (micro category)
    }
  }
  LDEBUG << "Creating SVM Tagger with model: " << m_d->m_model;

}

LimaStatusCode PerlSvmToolPosTagger::process(AnalysisContent& analysis) const
{
  Lima::TimeUtilsController timer("PerlSvmToolPosTagger");

  // start postagging here !
#ifdef DEBUG_LP
  PTLOGINIT;
  LINFO << "start SvmToolPosTager";
#endif
  // Retrieve morphosyntactic graph
  auto anagraph = static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph"));
  auto srcgraph = anagraph->getGraph();
  auto endVx = anagraph->lastVertex();

  /// Creates the posgraph with the second parameter (deleteTokenWhenDestroyed)
  /// set to false as the tokens are owned by the anagraph
  /// @note : tokens newly created later will be owned by their creator and have
  /// to be deleted by this one
  auto posgraph = new LinguisticAnalysisStructure::AnalysisGraph("PosGraph",
                                                                 m_d->m_language,
                                                                 false,
                                                                 true);
  analysis.setData("PosGraph",posgraph);

  /** Creation of an annotation graph if necessary*/
  AnnotationData* annotationData =
  static_cast< AnnotationData* >(analysis.getData("AnnotationData"));
  if (annotationData==0)
  {
    annotationData = new AnnotationData();
    /** Creates a node in the annotation graph for each node of the
    * morphosyntactic graph. Each new node is annotated with the name mrphv and
    * associated to the morphosyntactic vertex number */
    if (static_cast<AnalysisGraph*>(analysis.getData("AnalysisGraph")) != 0)
    {
      static_cast<AnalysisGraph*>(
        analysis.getData("AnalysisGraph"))->populateAnnotationGraph(
          annotationData,
          "AnalysisGraph");
    }
    analysis.setData("AnnotationData",annotationData);
  }

  // if graph is empty then do nothing
  // graph is empty if it has only 2 vertices, start (0) and end (0)
  if (num_vertices(*srcgraph)<=2)
  {
    return SUCCESS_ID;
  }
  VertexTokenPropertyMap tokens = get(vertex_token, *srcgraph);

  // Create postagging graph
  LinguisticGraph* resultgraph=posgraph->getGraph();
  remove_edge(posgraph->firstVertex(),posgraph->lastVertex(),*resultgraph);

  const auto& propertyCodeManager = static_cast<const LanguageData&>(
    MediaticData::single().mediaData(m_d->m_language)).getPropertyCodeManager();
  const auto& microManager = propertyCodeManager.getPropertyManager("MICRO");
  // to add tokens possible tags to the tagger dictionary
  const auto& propertyManagers = propertyCodeManager.getPropertyManagers();

  // TODO create a wrapper on the analysis graph to read tokens in a stream
  std::ostringstream oss("");
  LinguisticGraphVertex currentVx=anagraph->firstVertex();
  std::vector< LinguisticGraphVertex > anaVertices;
  while (currentVx != endVx)
  {
    if (currentVx != 0 && tokens[currentVx] != 0)
    {
      auto tok = tokens[currentVx];
      std::string token = tok->stringForm().toUtf8().constData();
      boost::replace_all(token, " ", "_");
      std::ostringstream lineoss("");
      lineoss << token << " (";
      auto morphoData = get(vertex_data,*srcgraph,currentVx);
      for (auto morphDataIt = morphoData->begin();
           morphDataIt != morphoData->end(); morphDataIt++)
      {
        if (morphDataIt != morphoData->begin())
        {
          lineoss << ",";
        }
        QString fullTag;
        QTextStream tagStream(&fullTag);
        auto tag = microManager.getPropertySymbolicValue(
            (*morphDataIt).properties);
        tagStream << QString::fromUtf8(tag.c_str());
        if (m_d->m_allFeatures ||!m_d->m_features.isEmpty())
        {
          QStringList features;
          for (auto propItr = propertyManagers.cbegin();
               propItr != propertyManagers.cend(); propItr++)
          {
            if (!propItr->second.getPropertyAccessor().empty((*morphDataIt).properties))
            {
              auto property = QString::fromUtf8(propItr->first.c_str());
              auto value = QString::fromUtf8(
                  propItr->second.getPropertySymbolicValue(
                      (*morphDataIt).properties).c_str());
              if (property != "MACRO" && property != "MICRO"
                  && (m_d->m_allFeatures || m_d->m_features.contains(property)))
              {
                features << QString("%1=%2").arg(property).arg(value);
              }
            }
          }
          features.sort();
          if (!features.isEmpty())
          {
            tagStream << "-";
            for (auto it = features.cbegin(); it != features.cend(); it++)
            {
              if (it != features.cbegin())
              {
                tagStream << "|";
              }
              tagStream << *it;
            }
          }
        }
        lineoss << fullTag.toUtf8().constData();;
      }
      lineoss << ")";
      lineoss << std::endl;
      oss << lineoss.str();
      anaVertices.push_back(currentVx);
    }
    LinguisticGraphOutEdgeIt it, it_end;
    boost::tie(it, it_end) = boost::out_edges(currentVx, *srcgraph);
    if (it != it_end)
    {
      currentVx = boost::target(*it, *srcgraph);
    }
    else
    {
      currentVx = endVx;
    }
  }
#ifdef DEBUG_LP
  LDEBUG << "Tagging '" << oss.str() << "'";
#endif
  QTemporaryFile input;
  if (!input.open())
  {
    PTLOGINIT;
    LERROR << "Error opening temporary file";
    return CANNOT_OPEN_FILE_ERROR;
  }

#ifdef DEBUG_LP
  LDEBUG << "Input temporary file is" << input.fileName();
#endif
  QTextStream inputs(&input);
  inputs << QString::fromUtf8(oss.str().c_str());

  input.close();
  QString inputFileName = input.fileName();
  QTemporaryFile output;
  if (!output.open())
  {
    PTLOGINIT;
    LERROR << "Error opening temporary file";
    return CANNOT_OPEN_FILE_ERROR;
  }

#ifdef DEBUG_LP
  LDEBUG << "Output temporary file is" << output.fileName();
#endif
  output.close();

  QProcess svmtool;
  svmtool.setStandardInputFile(input.fileName());
  svmtool.setStandardOutputFile(output.fileName());
  svmtool.setProgram(m_d->m_svmtool);
  svmtool.setArguments({m_d->m_model.c_str()});
  svmtool.start();
  if (!svmtool.waitForFinished())
  {
    PTLOGINIT;
    LERROR << "Error Executing Perl SVMTool" << svmtool.errorString();
    return CANNOT_OPEN_FILE_ERROR;
  }
  if (!output.open())
  {
    PTLOGINIT;
    LERROR << "Error reopening temporary file";
    return CANNOT_OPEN_FILE_ERROR;
  }
  std::vector<LinguisticGraphVertex>::size_type anaVerticesIndex = 0;
  LinguisticGraphVertex previousPosVertex = posgraph->firstVertex();
  while (anaVerticesIndex < anaVertices.size()
          && !output.atEnd())
  {
    QByteArray resultLine = output.readLine();
#ifdef DEBUG_LP
    LDEBUG << "Result line: '" << resultLine << "'";
#endif
    auto elements = resultLine.split(' ');
    if (elements.size() < 2)
    {
      PTLOGINIT;
      LERROR << "Error in SVMTagger result line: did not get 2 elements in '"
              << resultLine << "'";
      return UNKNOWN_ERROR;
    }
    auto anaVertex = anaVertices[anaVerticesIndex];
    auto currentAnaToken = tokens[anaVertex];
    std::string token = currentAnaToken->stringForm().toUtf8().constData();
    boost::replace_all(token, " ", "_");
    if (QString::fromUtf8(token.c_str()) != elements[0])
    {
      PTLOGINIT;
      LERROR << "Error in SVMTagger result alignement with analysis graph: got '"
              << elements[0] << "' from SVMTagger and '"
              << currentAnaToken->stringForm() << "' from graph";
      return UNKNOWN_ERROR;
    }

    auto newVx = boost::add_vertex(*resultgraph);
    annotationData->addMatching("PosGraph", newVx, "annot", anaVertex);
    annotationData->addMatching("AnalysisGraph", anaVertex, "PosGraph", newVx);
    auto agv =  annotationData->createAnnotationVertex();
    annotationData->annotate(agv, QString::fromUtf8("PosGraph"), newVx);

    // set linguistic infos
    auto morphoData = get(vertex_data,*srcgraph,anaVertex);
    auto srcToken = get(vertex_token,*srcgraph,anaVertex);
    if (morphoData!=0)
    {
      auto posData = new MorphoSyntacticData();
      CheckDifferentPropertyPredicate differentMicro(
          *m_d->m_microAccessor,
          microManager.getPropertyValue(elements[1].data()));
      std::back_insert_iterator<MorphoSyntacticData> backInsertItr(*posData);
      remove_copy_if(morphoData->begin(),
                     morphoData->end(),
                     backInsertItr,
                     differentMicro);
      if (posData->empty() || morphoData->empty())
      {
        PTLOGINIT;
        LWARN << "No matching category found for tagger result "
              << elements[0] << " " << elements[1];
        if (!morphoData->empty())
        {
          LWARN << "Taking any one";
          posData->push_back(morphoData->front());
        }
      }
      put(vertex_data,*resultgraph,newVx,posData);
      put(vertex_token,*resultgraph,newVx,srcToken);
    }

    boost::add_edge(previousPosVertex, newVx, *resultgraph);

    previousPosVertex = newVx;
    anaVerticesIndex++;
  }
  boost::add_edge(previousPosVertex, posgraph->lastVertex(), *resultgraph);

#ifdef DEBUG_LP
  LINFO << "PerlSvmToolPosTagger postagging done.";
#endif

  return SUCCESS_ID;
}

  typedef std::vector<StepData> StepDataVector;


} // PosTagger

} // LinguisticProcessing

} // Lima