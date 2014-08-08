#include "CorrelationMatrix.h"

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <QtCore/qmath.h>

#include "IException.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "SparseBlockMatrix.h"

namespace Isis {
  /**
   * Default Constructor
   */
  CorrelationMatrix::CorrelationMatrix() {
    m_covarianceFileName = new FileName(""); 
    m_correlationFileName = new FileName(""); 
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>();
    m_imagesAndParameters = new QMap<QString, QStringList>();
    m_diagonals = new QList<double>();
  }



  /**
   * This constructor will create a CorrelationMatrix object given a pvl.
   *
   * Object = CorrelationMatrix
   *
   *    CovarianceFileName = fileName.dat
   *    CorrelationFileName = fileName.dat
   *
   *    Group = ImagesAndParameters
   *       Image1 = "Parameter1", "Parameter2", "..."
   *       Image2 = "Parameter1", "Parameter2", "..."
   *       Image3 = "Parameter1", "Parameter2", "..."
   *    End_Group
   *
   * End_Object
   *
   */
  CorrelationMatrix::CorrelationMatrix(PvlObject storedMatrixData) {
    m_imagesAndParameters = NULL;
    m_covarianceFileName = NULL;
    m_correlationFileName = NULL;
    m_diagonals = NULL;
    m_visibleBlocks = NULL;

    if (storedMatrixData.name() != "CorrelationMatrixData") {
      QString msg = "This Pvl Object does not have the correct correlation information. The Object "
                    "you are looking for is called CorrelationMatrixData";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    try {
      m_covarianceFileName =
          new FileName(storedMatrixData.findKeyword("CovarianceMatrixFileName")[0]);
    }
    catch (IException &e) {
      QString msg = "Could not find the Covariance Matrix .dat file name.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
    
    try {
      QString corrFileName = storedMatrixData.findKeyword("CorrelationMatrixFileName")[0];
      if(corrFileName == "NULL") {
        m_correlationFileName = new FileName;
      }
      else {
        m_correlationFileName = new FileName(corrFileName);
      }
    }
    catch (IException &e) {
      QString msg = "Could not find the Correlation Matrix .dat file name.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    try {
      PvlObject::PvlKeywordIterator imgsIt = storedMatrixData.findGroup("ImagesAndParameters").begin();
      while ( imgsIt != storedMatrixData.findGroup("ImagesAndParameters").end() ) {
        QStringList params = (*imgsIt)[0].split(",");
        m_imagesAndParameters->insert(imgsIt->name(), params);
      }
    }
    catch (IException &e) {
      QString msg = "Could not get Images and Parameters from ImagesAndParameters group.";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }



  /**
   * Copy Constructor
   *
   * @param other The CorrelationMatrix to copy
   */
  CorrelationMatrix::CorrelationMatrix(const CorrelationMatrix &other) {
    m_imagesAndParameters = new QMap<QString, QStringList>(*other.m_imagesAndParameters);
    m_covarianceFileName = new FileName(*other.m_covarianceFileName);
    m_correlationFileName = new FileName(*other.m_correlationFileName);
    m_diagonals = new QList<double>(*other.m_diagonals);
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>(*other.m_visibleBlocks);
  }



  /**
   * Destructor
   */
  CorrelationMatrix::~CorrelationMatrix() {
    delete m_imagesAndParameters;
    m_imagesAndParameters = NULL;

    delete m_covarianceFileName;
    m_covarianceFileName = NULL;

    delete m_correlationFileName;
    m_correlationFileName = NULL;

    delete m_diagonals;
    m_diagonals = NULL;

    delete m_visibleBlocks;
    m_visibleBlocks = NULL;
  }



  /**
   * Equal Operator
   *
   * Should this call the copy constructor???
   *
   * @param other The matrix to assign to this matrix.
   * @return CorrelationMatrix This new matrix.
   */
  CorrelationMatrix &CorrelationMatrix::operator=(const CorrelationMatrix &other) {

    if (&other != this) {

      delete m_imagesAndParameters;
      m_imagesAndParameters = NULL;
      m_imagesAndParameters = new QMap<QString, QStringList>(*other.m_imagesAndParameters);
  
      delete m_covarianceFileName;
      m_covarianceFileName = NULL;
      m_covarianceFileName = new FileName(*other.m_covarianceFileName);
  
      delete m_correlationFileName;
      m_correlationFileName = NULL;
      m_correlationFileName = new FileName(*other.m_correlationFileName);
  
      delete m_diagonals;
      m_diagonals = NULL;
      m_diagonals = new QList<double>(*other.m_diagonals);
  
      delete m_visibleBlocks;
      m_visibleBlocks = NULL;
      m_visibleBlocks = new QList<SparseBlockColumnMatrix>(*other.m_visibleBlocks);

    }

    return *this;
  }



  /**
  * This mehtod reads the covariance matrix in from a file, one SparseBlockColumnMatrix at a time.
  * It then stores the diagonal values from that column and computes the correlation values. The
  * resulting SparseBlockMatrix is written to a new file, one SparseBlockColumnMatrix at a time.
  */
  void CorrelationMatrix::computeCorrelationMatrix() {

    if ( !isValid() ) {
      QString msg = "Cannot compute correlation matrix without a specified file name. Use "
                    "setCorrelationFileName(FileName) before calling computeCorrelationMatrix().";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    delete m_visibleBlocks;
    m_visibleBlocks = NULL;
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>;

    // Create file handle
    QFile matrixInput( m_covarianceFileName->expanded() );
    QFile matrixOutput( m_correlationFileName->expanded() );

    // Open file to write to
    matrixInput.open(QIODevice::ReadOnly);
    matrixOutput.open(QIODevice::WriteOnly);

    // Open Stream
    QDataStream inStream(&matrixInput);
    QDataStream outStream(&matrixOutput);

    double firstParam1 = 0, //starting param for each iteration
            firstParam2 = 0;
    double param1 = 0, // current param for each iteration
            param2 = 0;
    // Read one column at a time
    SparseBlockColumnMatrix sbcm;
    while( !inStream.atEnd() ) {
      inStream >> sbcm;

      // Store diagonal
      int numOfBlocks = sbcm.size();
      int lastBlock = numOfBlocks - 1;
      int numDiagonals = sbcm[lastBlock]->size1();

      // Get Diagonals
      for (int i = 0; i < numDiagonals; i++) {
        double val = ( *(sbcm[lastBlock]) )(i, i);
        m_diagonals->append(val);
      }

      // compute correlations
      QMapIterator<int, boost::numeric::ublas::matrix<double>*> block(sbcm);

      while ( block.hasNext() ) { // each block in the column
        block.next();
        for (int row = 0; row < (int)block.value()->size1(); row++) { // each row in the block
          for (int column = 0; column < (int)block.value()->size2(); column++) { // each column 
            // correlation = covariance / (variance1 * variance2)
            ( *block.value() )(row, column) = ( *block.value() )(row, column) /
                                              sqrt( (*m_diagonals)[param1] *
                                                    (*m_diagonals)[param2] );
            param2++;
          }
          param1++;
          param2 = firstParam2;
        }
        firstParam1 += block.value()->size1();
        param1 = firstParam1;
      }
      firstParam1 = 0;  // start each column at first element of diagonal list
      param1 = firstParam1;
      firstParam2 += block.value()->size2();
      param2 = firstParam2;

      outStream << sbcm;
      m_visibleBlocks->append(sbcm);
    }
    
    // close file
    matrixInput.close();
    matrixOutput.close();
  }



  /**
   * This method will open the correlation matrix file and read in the blocks that apply to
   *   the requested area. It will populate m_visibleElements.
   *
   * @param x fisrt coordinate of the location in the matrix that the user wants to see.
   * @param y second coordinate of the location in the matrix that the user wants to see.
   */
  void CorrelationMatrix::retrieveVisibleElements(int x, int y) {

//     if ( !correlationMatrixExists() ) {
      // call computeCorrelationMatrix
//     }
    // read the values we want from the correlation matrix file.

    // store values by column?
    // return list of values in m_visibleElements
  }



  /**
   * This is the public accessor for the list of elements that should be displayed in the current
   *   view.
   *
   * @return QList<MatrixElement*> The list of currently visible elements.
   *
   */
//   QList<MatrixElement*> CorrelationMatrix::visibleElements() {
//     return *m_visibleElements;
//   }



  /**
   * See if the correlation matrix has already been calculated by checking to see if
   *   the correlation matrix file has been created.
   *
   * @return true if the correlation matrix has already been set.
   */
  bool CorrelationMatrix::isValid() {
    return !(m_correlationFileName->name() == "" || m_covarianceFileName->name() == "");
  }



  /**
   * This is used to make sure the covariance matrix exists. If it doesn't this class is not
   * valid.
   *
   * If this file exists, we can compute the correlation matrix.
   *
   * @return bool True if the covariance matrix has been created.
   */
  bool CorrelationMatrix::hasCovMat() {
    return !(m_covarianceFileName->name() == "");
  }



  // Set Methods
  /**
   *Set the qmap of images and parameters.
   *
   * @param
   */
  void CorrelationMatrix::setCorrelationFileName(FileName correlationFileName) {
    if (m_correlationFileName == NULL) {
      m_correlationFileName = new FileName(correlationFileName);
    }
    else {
      *m_correlationFileName = correlationFileName;
    }
  }



  /**
   *Set the qmap of images and parameters.
   *
   * @param
   */
  void CorrelationMatrix::setCovarianceFileName(FileName covarianceFileName) {
    if (m_covarianceFileName == NULL) {
      m_covarianceFileName = new FileName(covarianceFileName);
    }
    else {
      *m_covarianceFileName = covarianceFileName;
    }
    //Make the correlation matrix file name match the covariance matrix file name.
    QString fName = covarianceFileName.expanded().replace( QString("inverse"),
                                                           QString("correlation") );
    setCorrelationFileName( FileName(fName) );
  }



  /**
   * Set the qmap of images and parameters.
   *
   * @param
   */
  void CorrelationMatrix::setImagesAndParameters(QMap<QString, QStringList> imagesAndParameters) {
    if (m_imagesAndParameters == NULL) {
      m_imagesAndParameters = new QMap<QString, QStringList>(imagesAndParameters);
    }
    else {
      *m_imagesAndParameters = imagesAndParameters;
    }      
  }



  /**
   * Public access for the correlation matrix file name.
   *
   * @return
   */
  FileName CorrelationMatrix::correlationFileName() {
    return *m_correlationFileName;
  }



  /**
   * Public access for the covariance matrix file name.
   *
   * @return
   */
  FileName CorrelationMatrix::covarianceFileName() {
    return *m_covarianceFileName;
  }



  /**
   * Public access for the qmap of images and parameters.
   *
   * @return
   */
  QMap<QString, QStringList> *CorrelationMatrix::imagesAndParameters() {
    return m_imagesAndParameters;
  }



  /**
   * This method will read the matrix in from the file and hold on to the whole thing.
   * This will only be used when the matrix is small enough that this will be useful.
   *
   */
  void CorrelationMatrix::getWholeMatrix() {
//     SparseBlockColumnMatrix sbcm;
//     QFile matrixInput( m_correlationFileName->expanded() );
//     matrixInput.open(QIODevice::ReadOnly);
//     QDataStream inStream(&matrixInput);
//
//     while( !inStream.atEnd() ) {
//       inStream >> sbcm;
//       m_visibleBlocks->append(&sbcm);
//     }
  }



  /**
   * This method will be used when the matrix is too big to display the whole thing.
   * It will read in the block we want to see and the two blocks for the diagonals that belong to
   * the right images.
   */
  void CorrelationMatrix::getThreeVisibleBlocks() {
  }



  /**
   * Get the visible part of the matrix.
   */
  QList<SparseBlockColumnMatrix> *CorrelationMatrix::visibleBlocks() {
    return m_visibleBlocks;
  }



  /**
   * This method creates a pvl group with the information necessary to recreate this correlation
   * matrix.
   *
   * Object = CorrelationMatrixData
   *   CovarianceMatrixFileName = /location/covarianceTmpFileName.dat
   *   CorrelationMatrixFileName = /location/correlationTmpFileName.dat
   * 
   *   Group = ImagesAndParameters
   *     Image1Name = "Param1, Param2, ..., ParamN"
   *     ...
   *     ImageNName = "..."
   *   End_Group
   * End_Object
   * 
   * @return PvlGroup The info needed to recreate this correlation matrix.
   */
  PvlObject CorrelationMatrix::pvlObject() {
    PvlObject corrMatInfo("CorrelationMatrixData");
    
    corrMatInfo += PvlKeyword( "CovarianceMatrixFileName", m_covarianceFileName->expanded() );
    corrMatInfo += PvlKeyword( "CorrelationMatrixFileName", m_correlationFileName->expanded() );

    PvlGroup imgsAndParams("ImagesAndParameters");
    QMapIterator<QString, QStringList> imgParamIt(*m_imagesAndParameters);
    while ( imgParamIt.hasNext() ) {
      imgParamIt.next();
      imgsAndParams += PvlKeyword( imgParamIt.key(), imgParamIt.value().join(",") );
    }
    corrMatInfo += imgsAndParams;
    
    return corrMatInfo;
  }



  QDataStream &CorrelationMatrix::write(QDataStream &stream) const {
    // QMaps
    stream << *m_imagesAndParameters;
    // FileNames
    stream << m_covarianceFileName->expanded() << m_correlationFileName->expanded();
    // QLists
    stream << *m_diagonals << *m_visibleBlocks;
    return stream;
  }



  QDataStream &CorrelationMatrix::read(QDataStream &stream) {
    // QMaps
    QMap<QString, QStringList> imagesAndParameters;
    stream >> imagesAndParameters;
    delete m_imagesAndParameters;
    m_imagesAndParameters = NULL;
    m_imagesAndParameters = new QMap<QString, QStringList>(imagesAndParameters);

    // FileNames
    QString covarianceFileName;
    stream >> covarianceFileName;
    delete m_covarianceFileName;
    m_covarianceFileName = NULL;
    m_covarianceFileName  = new FileName(covarianceFileName);

    QString correlationFileName;
    stream >>correlationFileName;
    delete m_correlationFileName;
    m_correlationFileName = NULL;
    m_correlationFileName = new FileName(correlationFileName);

    // QLists
    QList<double> diagonals;
    stream >> diagonals;
    delete m_diagonals;
    m_diagonals = NULL;
    m_diagonals = new QList<double>(diagonals);

    QList<SparseBlockColumnMatrix> visibleBlocks;
    stream << visibleBlocks;
    delete m_visibleBlocks;
    m_visibleBlocks = NULL;
    m_visibleBlocks = new QList<SparseBlockColumnMatrix>(visibleBlocks);

    return stream;
  }



  QDataStream &operator<<(QDataStream &stream, const CorrelationMatrix &matrix) {
    return matrix.write(stream);
  }



  QDataStream &operator>>(QDataStream &stream, CorrelationMatrix &matrix) {
    return matrix.read(stream);
  }
}