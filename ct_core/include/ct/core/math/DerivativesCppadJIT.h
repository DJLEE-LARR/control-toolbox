/**********************************************************************************************************************
This file is part of the Control Toobox (https://adrlab.bitbucket.io/ct), copyright by ETH Zurich, Google Inc.
Authors:  Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo, Farbod Farshidian
Licensed under Apache2 license (see LICENSE file in main directory)
**********************************************************************************************************************/

#pragma once

#include "CppadUtils.h"
#include "DerivativesCppadSettings.h"

namespace ct {
namespace core {

//! Jacobian using Auto-Diff Codegeneration
/*!
 * Uses Auto-Diff code generation to compute the Jacobian \f$ J(x_s) = \frac{df}{dx} |_{x=x_s} \f$ of
 * a regular vector-valued mathematical function \f$ y = f(x) \f$ .
 *
 * x has IN_DIM dimension and y has OUT_DIM dimension. Thus, they can be
 * scalar functions (IN_DIM = 1, OUT_DIM = 1), fixed or variable size
 * (IN_DIM = -1, OUT_DIM = -1) functions.
 *
 * \note In fact, this class is called Jacobian but computes also zero order derivatives
 *
 * @tparam IN_DIM Input dimensionality of the function (use Eigen::Dynamic (-1) for dynamic size)
 * @tparam OUT_DIM Output dimensionailty of the function (use Eigen::Dynamic (-1) for dynamic size)
 */
template <int IN_DIM, int OUT_DIM>
class DerivativesCppadJIT : public CppadUtils<IN_DIM, OUT_DIM>,
                            public Derivatives<IN_DIM, OUT_DIM, double>  // double on purpose!
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    typedef ADScalar AD_SCALAR;
    typedef ADCGScalar CG_SCALAR;         //!< CG_SCALAR  type
    typedef ADCGValueType CG_VALUE_TYPE;  //!< autodiff scalar type

    typedef Eigen::Matrix<AD_SCALAR, IN_DIM, 1> IN_TYPE_AD;    //!< function input vector type
    typedef Eigen::Matrix<AD_SCALAR, OUT_DIM, 1> OUT_TYPE_AD;  //!< function  output vector type

    typedef Eigen::Matrix<CG_SCALAR, IN_DIM, 1> IN_TYPE_CG;    //!< function input vector type
    typedef Eigen::Matrix<CG_SCALAR, OUT_DIM, 1> OUT_TYPE_CG;  //!< function  output vector type

    typedef Eigen::Matrix<double, IN_DIM, 1> IN_TYPE_D;         //!< function input vector type double
    typedef Eigen::Matrix<double, OUT_DIM, 1> OUT_TYPE_D;       //!< function output vector type
    typedef Eigen::Matrix<double, OUT_DIM, IN_DIM> JAC_TYPE_D;  //!< Jacobian type
    typedef Eigen::Matrix<double, OUT_DIM, IN_DIM, Eigen::RowMajor>
        JAC_TYPE_ROW_MAJOR;  //!< Jocobian type in row-major format
    typedef Eigen::Matrix<double, IN_DIM, IN_DIM> HES_TYPE_D;
    typedef Eigen::Matrix<double, IN_DIM, IN_DIM, Eigen::RowMajor> HES_TYPE_ROW_MAJOR;


    typedef std::function<OUT_TYPE_CG(const IN_TYPE_CG&)> FUN_TYPE_CG;  //!< function type
    typedef std::function<OUT_TYPE_AD(const IN_TYPE_AD&)> FUN_TYPE_AD;

    typedef CppadUtils<IN_DIM, OUT_DIM> Utils;
    typedef Derivatives<IN_DIM, OUT_DIM> DerivativesBase;

    /**
     * @brief      Contructs the derivatives for codegeneration using a
     *             FUN_TYPE_CG function
     * @warning    If IN_DIM and/our OUT_DIM are set to dynamic (-1), then the
     *             actual dimensions of x and y have to be passed here.
     *
     * @param      f          The function to be autodiffed
     * @param[in]  inputDim   inputDim input dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     * @param[in]  outputDim  outputDim output dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     */
    DerivativesCppadJIT(FUN_TYPE_CG& f, int inputDim = IN_DIM, int outputDim = OUT_DIM)
        : Utils(f, inputDim, outputDim), DerivativesBase(), compiled_(false), libName_("")
    {
    }

    /**
     * @brief      Constructs the derivatives for autodiff without
     *             codegeneration using a FUN_TYPE_AD function
     *
     * @warning    If IN_DIM and/our OUT_DIM are set to dynamic (-1), then the
     *             actual dimensions of x and y have to be passed here.
     *
     * @param      f          The function to be autodiffed
     * @param[in]  inputDim   inputDim input dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     * @param[in]  outputDim  outputDim output dimension, must be specified if
     *                        template parameter IN_DIM is -1 (dynamic)
     */
    DerivativesCppadJIT(FUN_TYPE_AD& f, int inputDim = IN_DIM, int outputDim = OUT_DIM)
        : Utils(f, inputDim, outputDim), DerivativesBase(), compiled_(false), libName_("")
    {
    }

    //! copy constructor
    DerivativesCppadJIT(const DerivativesCppadJIT& arg)
        : Utils(arg),
          DerivativesBase(arg),
          compiled_(arg.compiled_),
          libName_(arg.libName_),
          dynamicLib_(arg.dynamicLib_)
    {
        if (compiled_)
            model_ =
                std::shared_ptr<CppAD::cg::GenericModel<double>>(dynamicLib_->model("DerivativesCppad" + libName_));
    }

    virtual void updateDerived() override
    {
        compiled_ = false;
        libName_ = "";
        dynamicLib_ = nullptr;
        model_ = nullptr;
    }

    //! destructor
    virtual ~DerivativesCppadJIT() {}
    //! deep cloning of Jacobian
    DerivativesCppadJIT* clone() const { return new DerivativesCppadJIT<IN_DIM, OUT_DIM>(*this); }
    virtual OUT_TYPE_D forwardZero(const Eigen::VectorXd& x)
    {
        if (compiled_)
        {
            assert(model_->isForwardZeroAvailable() == true);
            return model_->ForwardZero(x);
        }
        else
            return this->adCppadFun_.Forward(0, x);
    }

    virtual JAC_TYPE_D jacobian(const Eigen::VectorXd& x)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");


        Eigen::VectorXd jac;
        if (compiled_)
        {
            assert(model_->isJacobianAvailable() == true);
            jac = model_->Jacobian(x);
        }
        else
            jac = this->adCppadFun_.Jacobian(x);

        JAC_TYPE_D out(this->outputDim_, x.rows());
        out = JAC_TYPE_ROW_MAJOR::Map(jac.data(), this->outputDim_, x.rows());
        return out;
    }

    virtual void sparseJacobian(const Eigen::VectorXd& x,
        Eigen::VectorXd& jac,
        Eigen::VectorXi& iRow,
        Eigen::VectorXi& jCol)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");

        if (compiled_)
        {
            assert(model_->isSparseJacobianAvailable() == true);
            std::vector<double> input(x.data(), x.data() + x.rows() * x.cols());
            std::vector<double> output;
            model_->SparseJacobian(input, output, sparsityRowsJacobian_, sparsityColsJacobian_);
            jac = Eigen::Map<Eigen::VectorXd>(output.data(), output.size(), 1);
            iRow = sparsityRowsJacobianEigen_;
            jCol = sparsityColsJacobianEigen_;
        }
        else
            jac = this->adCppadFun_.SparseJacobian(x);
    }

    virtual Eigen::VectorXd sparseJacobianValues(const Eigen::VectorXd& x)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");

        if (compiled_)
        {
            assert(model_->isSparseJacobianAvailable() == true);
            std::vector<double> input(x.data(), x.data() + x.rows() * x.cols());
            std::vector<double> output;
            model_->SparseJacobian(input, output, sparsityRowsJacobian_, sparsityColsJacobian_);
            return Eigen::Map<Eigen::VectorXd>(output.data(), output.size(), 1);
        }
        else
            return this->adCppadFun_.SparseJacobian(x);
    }


    virtual HES_TYPE_D hessian(const Eigen::VectorXd& x, const Eigen::VectorXd& lambda)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");

        if (compiled_)
        {
            assert(model_->isHessianAvailable() == true);
            Eigen::VectorXd hessian = model_->Hessian(x, lambda);
            HES_TYPE_D out(x.rows(), x.rows());
            out = HES_TYPE_ROW_MAJOR::Map(hessian.data(), x.rows(), x.rows());
            return out;
        }
        else
        {
            Eigen::VectorXd hessian = this->adCppadFun_.Hessian(x, lambda);
            HES_TYPE_D out(x.rows(), x.rows());
            out = HES_TYPE_ROW_MAJOR::Map(hessian.data(), x.rows(), x.rows());
            return out;
        }
    }

    virtual void sparseHessian(const Eigen::VectorXd& x,
        const Eigen::VectorXd& lambda,
        Eigen::VectorXd& hes,
        Eigen::VectorXi& iRow,
        Eigen::VectorXi& jCol)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");

        if (compiled_)
        {
            assert(model_->isSparseJacobianAvailable() == true);
            std::vector<double> input(x.data(), x.data() + x.rows() * x.cols());
            std::vector<double> inputLambda(lambda.data(), lambda.data() + lambda.rows() * lambda.cols());
            std::vector<double> output;
            model_->SparseHessian(input, inputLambda, output, sparsityRowsHessian_, sparsityColsHessian_);
            hes = Eigen::Map<Eigen::VectorXd>(output.data(), output.size(), 1);
            iRow = sparsityRowsHessianEigen_;
            jCol = sparsityColsHessianEigen_;
        }
        else
            hes = this->adCppadFun_.SparseHessian(x, lambda);
    }


    virtual Eigen::VectorXd sparseHessianValues(const Eigen::VectorXd& x, const Eigen::VectorXd& lambda)
    {
        if (this->outputDim_ <= 0)
            throw std::runtime_error("Outdim dim smaller 0; Define output dim in DerivativesCppad constructor");

        if (compiled_)
        {
            assert(model_->isSparseHessianAvailable() == true);
            std::vector<double> input(x.data(), x.data() + x.rows() * x.cols());
            std::vector<double> inputLambda(lambda.data(), lambda.data() + lambda.rows() * lambda.cols());
            std::vector<double> output;
            model_->SparseHessian(input, inputLambda, output, sparsityRowsHessian_, sparsityColsHessian_);
            return Eigen::Map<Eigen::VectorXd>(output.data(), output.size(), 1);
        }
        else
            return this->adCppadFun_.SparseHessian(x, lambda);
    }

    //! get Jacobian sparsity pattern
    /*!
     * Auto-Diff automatically detects the sparsity pattern of the Jacobian. This method returns the pattern.
     * @return Sparsity pattern of the Jacobian
     */
    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> getSparsityPatternJacobian()
    {
        assert(model_->isJacobianSparsityAvailable() == true);

        std::vector<bool> sparsityVec = model_->JacobianSparsityBool();
        Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> sparsityMat(this->outputDim_, this->inputDim_);

        assert((int)(sparsityVec.size()) == this->outputDim_ * this->inputDim_);
        for (size_t row = 0; row < this->outputDim_; ++row)
            for (size_t col = 0; col < this->inputDim_; ++col)
                sparsityMat(row, col) = sparsityVec[col + row * this->inputDim_];

        return sparsityMat;
    }

    /**
     * @brief      get Hessian sparsity pattern
     *
     * @return     Auto-diff automatically detects the sparsity pattern of the Hessian
     */
    Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> getSparsityPatternHessian()
    {
        assert(model_->isHessianSparsityAvailable() == true);

        std::vector<bool> sparsityVec = model_->HessianSparsityBool();
        Eigen::Matrix<bool, Eigen::Dynamic, Eigen::Dynamic> sparsityMat(this->inputDim_, this->inputDim_);

        assert(sparsityVec.size() == this->inputDim_ * this->inputDim_);
        for (size_t row = 0; row < this->inputDim_; ++row)
            for (size_t col = 0; col < this->inputDim_; ++col)
            {
                // std::cout << "sparsityVec: " << sparsityRowsHessian_[col + row * this->inputDim_] << std::endl;
                sparsityMat(row, col) = sparsityVec[col + row * this->inputDim_];
            }

        return sparsityMat;
    }


    //! get Jacobian sparsity pattern
    /*!
     * Auto-Diff automatically detects the sparsity pattern of the Jacobian. This method returns the pattern
     * in row-column format. Row and columns contain the indeces of all non-zero entries.
     *
     * @param rows row indeces of non-zero entries
     * @param columns column indeces of non-zero entries
     */
    void getSparsityPatternJacobian(Eigen::VectorXi& rows, Eigen::VectorXi& columns)
    {
        assert(model_->isJacobianSparsityAvailable() == true);

        rows = sparsityRowsJacobianEigen_;
        columns = sparsityColsJacobianEigen_;
    }

    /**
     * @brief      Returns the number of nonzeros in the sparse jacobian
     *
     * @return     The number of nonzeros in the sparse jacobian
     */
    size_t getNumNonZerosJacobian()
    {
        assert(model_->isJacobianSparsityAvailable() == true);
        return sparsityRowsJacobian_.size();
    }

    /**
     * @brief      Returns the number of nonzeros in the sparse hessian
     *
     * @return     The number of non zeros in the sparse hessian
     */
    size_t getNumNonZerosHessian()
    {
        assert(model_->isJacobianSparsityAvailable() == true);
        return sparsityRowsHessian_.size();
    }

    //! get Hessian sparsity pattern
    /*!
     * Auto-Diff automatically detects the sparsity pattern of the Jacobian. This method returns the pattern
     * in row-column format. Row and columns contain the indeces of all non-zero entries.
     *
     * @param rows row indeces of non-zero entries
     * @param columns column indeces of non-zero entries
     */
    void getSparsityPatternHessian(Eigen::VectorXi& rows, Eigen::VectorXi& columns)
    {
        assert(model_->isHessianSparsityAvailable() == true);

        rows = sparsityRowsHessianEigen_;
        columns = sparsityColsHessianEigen_;
    }


    //! Uses just-in-time compilation to compile the Jacobian and other derivatives
    /*!
     *  This method generates source code for the Jacobian and zero order derivative. It then compiles
     *  the source code to a dynamically loadable library that then gets loaded.
     */
    void compileJIT(const DerivativesCppadSettings& settings,
        const std::string& libName = "unnamedLib",
        bool verbose = false)
    {
        if (compiled_)
            return;

        // assigning a unique identifier to the library in order to avoid race conditions in JIT
        libName_ = libName + std::to_string(std::hash<std::thread::id>()(std::this_thread::get_id()));

        CppAD::cg::ModelCSourceGen<double> cgen(this->cgCppadFun_, "DerivativesCppad" + libName_);

        cgen.setMultiThreading(settings.multiThreading_);
        cgen.setCreateForwardZero(settings.createForwardZero_);
        cgen.setCreateForwardOne(settings.createForwardOne_);
        cgen.setCreateReverseOne(settings.createReverseOne_);
        cgen.setCreateReverseTwo(settings.createReverseTwo_);
        cgen.setCreateJacobian(settings.createJacobian_);
        cgen.setCreateSparseJacobian(settings.createSparseJacobian_);
        cgen.setCreateHessian(settings.createHessian_);
        cgen.setCreateSparseHessian(settings.createSparseHessian_);
        cgen.setMaxAssignmentsPerFunc(settings.maxAssignements_);

        CppAD::cg::ModelLibraryCSourceGen<double> libcgen(cgen);
        std::string tempDir = "cppad_temp" + std::to_string(std::hash<std::thread::id>()(std::this_thread::get_id()));
        if (verbose)
        {
            std::cout << "Starting to compile " << libName_ << " library ..." << std::endl;
            std::cout << "In temporary directory " << tempDir << std::endl;
        }

        // compile source code
        CppAD::cg::DynamicModelLibraryProcessor<double> p(libcgen, libName_);
        if (settings.compiler_ == DerivativesCppadSettings::GCC)
        {
            CppAD::cg::GccCompiler<double> compiler;
            compiler.setTemporaryFolder(tempDir);
            dynamicLib_ = std::shared_ptr<CppAD::cg::DynamicLib<double>>(p.createDynamicLibrary(compiler));
        }

        else if (settings.compiler_ == DerivativesCppadSettings::CLANG)
        {
            CppAD::cg::ClangCompiler<double> compiler;
            compiler.setTemporaryFolder(tempDir);
            dynamicLib_ = std::shared_ptr<CppAD::cg::DynamicLib<double>>(p.createDynamicLibrary(compiler));
        }

        if (settings.generateSourceCode_)
        {
            CppAD::cg::SaveFilesModelLibraryProcessor<double> p2(libcgen);
            p2.saveSources();
        }

        model_ = std::shared_ptr<CppAD::cg::GenericModel<double>>(dynamicLib_->model("DerivativesCppad" + libName_));

        compiled_ = true;

        if (verbose)
            std::cout << "Successfully compiled " << libName_ << std::endl;

        if (model_->isJacobianSparsityAvailable())
        {
            model_->JacobianSparsity(sparsityRowsJacobian_, sparsityColsJacobian_);
            sparsityRowsJacobianEigen_.resize(sparsityRowsJacobian_.size());  //rowsTmp.resize(rowsVec.size());
            sparsityColsJacobianEigen_.resize(sparsityColsJacobian_.size());  //colsTmp.resize(rowsVec.size());

            Eigen::Matrix<size_t, Eigen::Dynamic, 1> rowsSizeT;
            rowsSizeT.resize(sparsityRowsJacobian_.size());
            Eigen::Matrix<size_t, Eigen::Dynamic, 1> colsSizeT;
            colsSizeT.resize(sparsityColsJacobian_.size());

            rowsSizeT = Eigen::Map<Eigen::Matrix<size_t, Eigen::Dynamic, 1>>(
                sparsityRowsJacobian_.data(), sparsityRowsJacobian_.size(), 1);
            colsSizeT = Eigen::Map<Eigen::Matrix<size_t, Eigen::Dynamic, 1>>(
                sparsityColsJacobian_.data(), sparsityColsJacobian_.size(), 1);

            sparsityRowsJacobianEigen_ = rowsSizeT.cast<int>();
            sparsityColsJacobianEigen_ = colsSizeT.cast<int>();
        }

        if (model_->isHessianSparsityAvailable())
        {
            model_->HessianSparsity(sparsityRowsHessian_, sparsityColsHessian_);
            sparsityRowsHessianEigen_.resize(sparsityRowsHessian_.size());  //rowsTmp.resize(rowsVec.size());
            sparsityColsHessianEigen_.resize(sparsityColsHessian_.size());  //colsTmp.resize(rowsVec.size());

            Eigen::Matrix<size_t, Eigen::Dynamic, 1> rowsSizeT;
            rowsSizeT.resize(sparsityRowsHessian_.size());
            Eigen::Matrix<size_t, Eigen::Dynamic, 1> colsSizeT;
            colsSizeT.resize(sparsityColsHessian_.size());

            rowsSizeT = Eigen::Map<Eigen::Matrix<size_t, Eigen::Dynamic, 1>>(
                sparsityRowsHessian_.data(), sparsityRowsHessian_.size(), 1);
            colsSizeT = Eigen::Map<Eigen::Matrix<size_t, Eigen::Dynamic, 1>>(
                sparsityColsHessian_.data(), sparsityColsHessian_.size(), 1);

            sparsityRowsHessianEigen_ = rowsSizeT.cast<int>();
            sparsityColsHessianEigen_ = colsSizeT.cast<int>();
        }
    }

private:
    bool compiled_;        //! flag if Jacobian is compiled
    std::string libName_;  //! a unique name for this library

    std::vector<size_t> sparsityRowsJacobian_;
    std::vector<size_t> sparsityColsJacobian_;
    std::vector<size_t> sparsityRowsHessian_;
    std::vector<size_t> sparsityColsHessian_;

    Eigen::VectorXi sparsityRowsJacobianEigen_;
    Eigen::VectorXi sparsityColsJacobianEigen_;
    Eigen::VectorXi sparsityRowsHessianEigen_;
    Eigen::VectorXi sparsityColsHessianEigen_;
    //!
    CppAD::cg::GccCompiler<double> compiler_;  //! compile for codegeneration
    CppAD::cg::ClangCompiler<double> compilerClang_;
    std::shared_ptr<CppAD::cg::DynamicLib<double>> dynamicLib_;  //! dynamic library to load after compilation
    std::shared_ptr<CppAD::cg::GenericModel<double>> model_;     //! the model
};

} /* namespace core */
} /* namespace ct */