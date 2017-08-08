/***********************************************************************************
Copyright (c) 2017, Michael Neunert, Markus Giftthaler, Markus Stäuble, Diego Pardo,
Farbod Farshidian. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
 * Neither the name of ETH ZURICH nor the names of its contributors may be used
      to endorse or promote products derived from this software without specific
      prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL ETH ZURICH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************************/

#include <iostream>
#include <cstdlib>

#include <ct/core/core.h>

#include <gtest/gtest.h>

using namespace ct::core;



TEST(DiscreteArrayTest, UnaryPlusMinusTest)
{
	const size_t nEl = 10;
	const size_t state_dim = 2;

	//! create two state vector array and fill them with random elements
	StateVectorArray<state_dim> array1 (nEl);
	StateVectorArray<state_dim> array2 (nEl);

	for (size_t i = 0; i<nEl; i++){
		array1[i].setRandom();
		array2[i].setRandom();
	}

	//! create backups for for later comparison
	StateVectorArray<state_dim> array1_backup = array1;
	StateVectorArray<state_dim> array2_backup = array2;


	//! test the overloaded operators
	StateVectorArray<state_dim> array_sum = array1 + array2;
	StateVectorArray<state_dim> array_diff = array1 - array2;

	for(size_t i = 0; i<nEl; i++){

		//! check that the summation is correct
		ASSERT_EQ(array_sum[i], (array1[i] + array2[i]));

		//! check that the difference is correct
		ASSERT_EQ(array_diff[i], (array1[i] - array2[i]));

		//! check that original elements were not altered
		ASSERT_EQ(array1[i], array1_backup[i]);
		ASSERT_EQ(array2[i], array2_backup[i]);
	}
}


TEST(DiscreteArrayTest, AddAssignTest)
{
	const size_t nEl = 10;
	const size_t state_dim = 2;

	//! create two state vector array and fill them with random elements
	StateVectorArray<state_dim> array1 (nEl);
	StateVectorArray<state_dim> array2 (nEl);

	for (size_t i = 0; i<nEl; i++){
		array1[i].setRandom();
		array2[i].setRandom();
	}

	//! create backups for for later comparison
	StateVectorArray<state_dim> array1_backup = array1;
	StateVectorArray<state_dim> array2_backup = array2;


	//! test the overloaded operator
	array1 += array2;

	for(size_t i = 0; i<nEl; i++){

		//! check that the summation is correct
		ASSERT_EQ(array1[i], (array1_backup[i] + array2_backup[i]));

		//! check that original rhs elements were not altered
		ASSERT_EQ(array2[i], array2_backup[i]);
	}
}


TEST(DiscreteArrayTest, SubstractAssignTest)
{
	const size_t nEl = 10;
	const size_t state_dim = 2;

	//! create two state vector array and fill them with random elements
	StateVectorArray<state_dim> array1 (nEl);
	StateVectorArray<state_dim> array2 (nEl);

	for (size_t i = 0; i<nEl; i++){
		array1[i].setRandom();
		array2[i].setRandom();
	}

	//! create backups for for later comparison
	StateVectorArray<state_dim> array1_backup = array1;
	StateVectorArray<state_dim> array2_backup = array2;


	//! test the overloaded operator
	array1 -= array2;

	for(size_t i = 0; i<nEl; i++){

		//! check that the summation is correct
		ASSERT_EQ(array1[i], (array1_backup[i] - array2_backup[i]));

		//! check that original rhs elements were not altered
		ASSERT_EQ(array2[i], array2_backup[i]);
	}
}


TEST(DiscreteArrayTest, MultiplicationTest)
{
	const size_t nEl = 10;
	const size_t state_dim = 2;

	StateVectorArray<state_dim> array1 (nEl);

	for (size_t i = 0; i<nEl; i++){
		array1[i].setRandom();
	}

	//! create backups for for later comparison
	StateVectorArray<state_dim> array1_backup = array1;

	double scalar = 3.141;

	//! test the overloaded operator
	StateVectorArray<state_dim> result = array1 * scalar;

	for(size_t i = 0; i<nEl; i++){

		//! check that the summation is correct
		ASSERT_EQ(result[i], (array1_backup[i] * scalar));

		//! check that original rhs elements were not altered
		ASSERT_EQ(array1[i], array1_backup[i]);
	}
}


TEST(DiscreteArrayTest, DivisionTest)
{
	const size_t nEl = 10;
	const size_t state_dim = 2;

	StateVectorArray<state_dim> array1 (nEl);

	for (size_t i = 0; i<nEl; i++){
		array1[i].setRandom();
	}

	//! create backups for for later comparison
	StateVectorArray<state_dim> array1_backup = array1;

	double scalar = 3.141;

	//! test the overloaded operator
	StateVectorArray<state_dim> result = array1 / scalar;

	for(size_t i = 0; i<nEl; i++){

		//! check that the summation is correct
		ASSERT_EQ(result[i], (array1_backup[i] / scalar));

		//! check that original rhs elements were not altered
		ASSERT_EQ(array1[i], array1_backup[i]);
	}
}


int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
