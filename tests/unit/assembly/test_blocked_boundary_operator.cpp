// Copyright (C) 2011 by the BEM++ Authors
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "../type_template.hpp"
#include "../check_arrays_are_close.hpp"

#include "bempp/common/config_ahmed.hpp"
#include "bempp/common/eigen_support.hpp"
#include "assembly/blocked_boundary_operator.hpp"
#include "assembly/blocked_operator_structure.hpp"
#include "assembly/context.hpp"
#include "assembly/discrete_boundary_operator.hpp"
#include "assembly/identity_operator.hpp"
#include "assembly/laplace_3d_single_layer_boundary_operator.hpp"
#include "assembly/modified_helmholtz_3d_single_layer_boundary_operator.hpp"
#include "assembly/numerical_quadrature_strategy.hpp"
#include "grid/grid_factory.hpp"
#include "grid/grid.hpp"
#include "space/piecewise_constant_scalar_space.hpp"
#include "space/piecewise_linear_continuous_scalar_space.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/type_traits/is_complex.hpp>

using namespace Bempp;

// Tests

BOOST_AUTO_TEST_SUITE(BlockedBoundaryOperator)

BOOST_AUTO_TEST_CASE_TEMPLATE(
    blocked_boundary_operator_produces_correct_weak_form_for_1x1_operator,
    ValueType, result_types) {
  // space | PL
  // ------+---
  // PC    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pwiseConstants(
      new PiecewiseConstantScalarSpace<BFT>(grid));
  shared_ptr<Space<BFT>> pwiseLinears(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid));


  AssemblyOptions assemblyOptions;
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(
          context, pwiseLinears, pwiseLinears, pwiseConstants);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> nonblockedWeakForm = op00.weakForm()->asMatrix();
  Matrix<RT> blockedWeakForm = blockedOp.weakForm()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, blockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    blocked_boundary_operator_produces_correct_weak_form_for_2x1_operator,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));

  AssemblyOptions assemblyOptions;
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc1, pl1);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm(mat00.rows()+mat10.rows(),mat00.cols());
  nonblockedWeakForm << mat00,
                        mat10;
  Matrix<RT> blockedWeakForm = blockedOp.weakForm()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, blockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    blocked_boundary_operator_produces_correct_weak_form_for_2x3_operator,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  V  |  V  |  V
  // PL2    |  V  |  V  |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(0, 1, op01);
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  structure.setBlock(1, 2, op12);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      eigenJoinRows(eigenJoinRows(eigenJoinCols(mat00, mat10),
                                      eigenJoinCols(mat01, mat11)),
                      eigenJoinCols(mat02, mat12));
  Matrix<RT> blockedWeakForm = blockedOp.weakForm()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, blockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    blocked_boundary_operator_produces_correct_weak_form_for_2x3_operator_with_empty_blocks,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  0  |  0  |  V
  // PL2    |  V  |  V  |  0

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = 0. * op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = 0. * op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = 0. * op12.weakForm()->asMatrix();

  Matrix<RT> nonblockedWeakForm =
      eigenJoinRows(eigenJoinRows(eigenJoinCols(mat00, mat10),
                                      eigenJoinCols(mat01, mat11)),
                      eigenJoinCols(mat02, mat12));
  Matrix<RT> blockedWeakForm = blockedOp.weakForm()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, blockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

#ifdef WITH_AHMED

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_1x1_operator,
    ValueType, result_types) {
  // space | PL
  // ------+---
  // PC    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pwiseConstants(
      new PiecewiseConstantScalarSpace<BFT>(grid));
  shared_ptr<Space<BFT>> pwiseLinears(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(
          context, pwiseLinears, pwiseLinears, pwiseConstants);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> nonblockedWeakForm = op00.weakForm()->asMatrix();
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()->asDiscreteAcaBoundaryOperator()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc1, pl1);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()->asDiscreteAcaBoundaryOperator()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  V  |  V  |  V
  // PL2    |  V  |  V  |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(0, 1, op01);
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  structure.setBlock(1, 2, op12);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()->asDiscreteAcaBoundaryOperator()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_empty_blocks,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  0  |  0  |  V
  // PL2    |  V  |  V  |  0

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = 0. * op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = 0. * op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = 0. * op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()->asDiscreteAcaBoundaryOperator()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator_containing_sparse_block,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  I

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      identityOperator<BFT, RT>(context, pl0, pc0, pl0);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()->asDiscreteAcaBoundaryOperator()->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_1x1_operator_with_interleaving,
    ValueType, result_types) {
  // space | PL
  // ------+---
  // PC    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pwiseConstants(
      new PiecewiseConstantScalarSpace<BFT>(grid));
  shared_ptr<Space<BFT>> pwiseLinears(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(
          context, pwiseLinears, pwiseLinears, pwiseConstants);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> nonblockedWeakForm = op00.weakForm()->asMatrix();
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator_with_interleaving,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc1, pl1);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  V  |  V  |  V
  // PL2    |  V  |  V  |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(0, 1, op01);
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  structure.setBlock(1, 2, op12);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_empty_blocks_with_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  0  |  0  |  V
  // PL2    |  V  |  V  |  0

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-2.msh",
      false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented-shifted-on-x-by-4.msh",
      false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op01 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pl0, pc0);
  BoundaryOperator<BFT, RT> op02 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pc2, pl2);
  BoundaryOperator<BFT, RT> op11 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pc1, pc2, pl2);
  BoundaryOperator<BFT, RT> op12 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl2, pc2, pl2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = 0. * op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = 0. * op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = 0. * op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator_containing_sparse_block_with_interleaving,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  I

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/cube-12-reoriented.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(context, pl0, pl0, pc0);
  BoundaryOperator<BFT, RT> op10 =
      identityOperator<BFT, RT>(context, pl0, pc0, pl0);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_1x1_operator_with_small_blocks_and_interleaving,
    ValueType, result_types) {
  // space | PL
  // ------+---
  // PC    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pwiseConstants(
      new PiecewiseConstantScalarSpace<BFT>(grid));
  shared_ptr<Space<BFT>> pwiseLinears(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.maximumBlockSize = 4;
  acaOptions.minimumBlockSize = 2;
  acaOptions.outputPostscript = true;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(
          context, pwiseLinears, pwiseLinears, pwiseConstants);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> nonblockedWeakForm = op00.weakForm()->asMatrix();
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator_with_with_small_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1-shifted-by-3.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.maximumBlockSize = 4;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc1, pl1, 0.2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_with_small_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  V  |  V  |  V
  // PL2    |  V  |  V  |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1-shifted-by-3.msh", false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1-shifted-by-6.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.maximumBlockSize = 4;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op01 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pl0, pc0, 0.2);
  BoundaryOperator<BFT, RT> op02 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pl0, pc0, 0.3);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc2, pl2, 0.4);
  BoundaryOperator<BFT, RT> op11 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pc2, pl2, 0.5);
  BoundaryOperator<BFT, RT> op12 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pc2, pl2, 0.6);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(0, 1, op01);
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  structure.setBlock(1, 2, op12);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_empty_blocks_with_with_small_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  0  |  0  |  V
  // PL2    |  V  |  V  |  0

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1-shifted-by-3.msh", false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-1-shifted-by-6.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.maximumBlockSize = 4;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op01 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pl0, pc0, 0.2);
  BoundaryOperator<BFT, RT> op02 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pl0, pc0, 0.3);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc2, pl2, 0.4);
  BoundaryOperator<BFT, RT> op11 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pc2, pl2, 0.5);
  BoundaryOperator<BFT, RT> op12 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pc2, pl2, 0.6);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = 0. * op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = 0. * op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = 0. * op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_1x1_operator_with_low_rank_blocks_and_interleaving,
    ValueType, result_types) {
  // space | PL
  // ------+---
  // PC    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pwiseConstants(
      new PiecewiseConstantScalarSpace<BFT>(grid));
  shared_ptr<Space<BFT>> pwiseLinears(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  acaOptions.outputPostscript = true;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      laplace3dSingleLayerBoundaryOperator<BFT, RT>(
          context, pwiseLinears, pwiseLinears, pwiseConstants);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> nonblockedWeakForm = op00.weakForm()->asMatrix();
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x1_operator_with_low_rank_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0
  // -------+---
  // PC0    |  V
  // PL1    |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2-shifted-by-3.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc1, pl1, 0.2);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(1, 0, op10);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm = arma::join_cols(mat00, mat10);
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_low_rank_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  V  |  V  |  V
  // PL2    |  V  |  V  |  V

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2-shifted-by-3.msh", false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2-shifted-by-6.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op01 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pl0, pc0, 0.2);
  BoundaryOperator<BFT, RT> op02 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pl0, pc0, 0.3);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc2, pl2, 0.4);
  BoundaryOperator<BFT, RT> op11 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pc2, pl2, 0.5);
  BoundaryOperator<BFT, RT> op12 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pc2, pl2, 0.6);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 0, op00);
  structure.setBlock(0, 1, op01);
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  structure.setBlock(1, 2, op12);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    asDiscreteAcaBoundaryOperator_produces_correct_weak_form_for_2x3_operator_with_low_rank_blocks_with_small_blocks_and_interleaving,
    ValueType, result_types) {
  // space  | PL0 | PC1 | PL2
  // -------+-----+-----+----
  // PC0    |  0  |  0  |  V
  // PL2    |  V  |  V  |  0

  typedef ValueType RT;
  typedef typename ScalarTraits<ValueType>::RealType RealType;
  typedef RealType BFT;

  GridParameters params;
  params.topology = GridParameters::TRIANGULAR;
  shared_ptr<Grid> grid0 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2.msh", false /* verbose */);
  shared_ptr<Grid> grid1 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2-shifted-by-3.msh", false /* verbose */);
  shared_ptr<Grid> grid2 = GridFactory::importGmshGrid(
      params, "meshes/sphere-ico-2-shifted-by-6.msh", false /* verbose */);

  shared_ptr<Space<BFT>> pc0(new PiecewiseConstantScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pl0(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid0));
  shared_ptr<Space<BFT>> pc1(new PiecewiseConstantScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pl1(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid1));
  shared_ptr<Space<BFT>> pc2(new PiecewiseConstantScalarSpace<BFT>(grid2));
  shared_ptr<Space<BFT>> pl2(
      new PiecewiseLinearContinuousScalarSpace<BFT>(grid2));

  AssemblyOptions assemblyOptions;
  AcaOptions acaOptions;
  acaOptions.minimumBlockSize = 2;
  assemblyOptions.switchToAcaMode(acaOptions);
  assemblyOptions.setVerbosityLevel(VerbosityLevel::LOW);
  shared_ptr<NumericalQuadratureStrategy<BFT, RT>> quadStrategy(
      new NumericalQuadratureStrategy<BFT, RT>);
  shared_ptr<Context<BFT, RT>> context(
      new Context<BFT, RT>(quadStrategy, assemblyOptions));

  BoundaryOperator<BFT, RT> op00 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pl0, pc0, 0.1);
  BoundaryOperator<BFT, RT> op01 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pl0, pc0, 0.2);
  BoundaryOperator<BFT, RT> op02 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pl0, pc0, 0.3);
  BoundaryOperator<BFT, RT> op10 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl0, pc2, pl2, 0.4);
  BoundaryOperator<BFT, RT> op11 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pc1, pc2, pl2, 0.5);
  BoundaryOperator<BFT, RT> op12 =
      modifiedHelmholtz3dSingleLayerBoundaryOperator<BFT, BFT, RT>(
          context, pl2, pc2, pl2, 0.6);

  BlockedOperatorStructure<BFT, RT> structure;
  structure.setBlock(0, 2, op02);
  structure.setBlock(1, 0, op10);
  structure.setBlock(1, 1, op11);
  Bempp::BlockedBoundaryOperator<BFT, RT> blockedOp(structure);

  Matrix<RT> mat00 = 0. * op00.weakForm()->asMatrix();
  Matrix<RT> mat01 = 0. * op01.weakForm()->asMatrix();
  Matrix<RT> mat02 = op02.weakForm()->asMatrix();
  Matrix<RT> mat10 = op10.weakForm()->asMatrix();
  Matrix<RT> mat11 = op11.weakForm()->asMatrix();
  Matrix<RT> mat12 = 0. * op12.weakForm()->asMatrix();
  Matrix<RT> nonblockedWeakForm =
      arma::join_rows(arma::join_rows(arma::join_cols(mat00, mat10),
                                      arma::join_cols(mat01, mat11)),
                      arma::join_cols(mat02, mat12));
  Matrix<RT> acaBlockedWeakForm =
      blockedOp.weakForm()
          ->asDiscreteAcaBoundaryOperator(-1, -1, true /*interleave*/)
          ->asMatrix();

  BOOST_CHECK(check_arrays_are_close<ValueType>(
      nonblockedWeakForm, acaBlockedWeakForm,
      10. * std::numeric_limits<RealType>::epsilon()));
}

#endif // WITH_AHMED
BOOST_AUTO_TEST_SUITE_END()
