// --------------------------------------------------------------------------------------------------------------------------------
// License:	BSD-3-Clause
// --------------------------------------------------------------------------------------------------------------------------------
//
// Copyright 2019      Armin Montigny
//
// --------------------------------------------------------------------------------------------------------------------------------
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met :
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the 
//    following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the 
//    following disclaimer in the documentation and /or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or 
//    promote products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------------------------------------------------------------



// This is a functor for Petricks Method
//
// Petricks method is an old and well defined deterministic algorithm, to solve
// the unate covering/set cover problem. It is np-complete which has a big impact
// on runtime and memory consumption.
//
// Perticks method is very old and was originally implemented in boolean minimization
// algorithms, especially for the reduction of prime implicant teables. But it can be used
// also for other cases. In general, for set cover problems.
//
// In this software we will use ist for prime implicant table reduction and for
// the slection of minimum MCDC test tests.
//
// One use case is already described in a different module and will be shown also here
// Given a Pime Implicant Chart, that cannot be reduced further with conventional
// methods, becuase it contains a cyclic core:
//
//     Ac Bc Ab bC aB aC 
//  3   X  X
//  5         X  X
//  7   X     X
//  9               X  X
// 11      X        X
// 13            X     X
//
// This cannot be reduced further. The result is called "cyclic core". That means, we will have
// more than one coverage sets, all to be combined with the essential prime implicant AD
//
// To resolve the cyclic core we will use Petricks Method
// 
// Every Prime Implicant will recieve a number, a index starting with 1. Then we add a P for Prime implicant
// before that index. Then we create an artificial boolean expression in conjunctive normal form (CNF) in that way, 
// that the resulting CNF will cover all minterms:
// So Ac=P1 Bc=P2 Ab=P3 bC=P4 aB=P5 aC=P6
// We build the Maxterms for each row: 
// 3: P1+P2  5: P3+4  7: P1+P3  9: P5+P6  11: P2+P5  13:  P4+P6
// CNF:  PALL = (P1+P2)(P3+P4)(P1+P3)(P5+P6)(P2+P5)(P4+P6)
// If Pall = 1 then we found the coverage sets. We transorm this CNF to a DNF by using monotone laws of boolean algebra.
// Multiplying out (Distributive law) through simple concatenation of literals (Minterms)
// and applying the idempotence and absorption laws by using STL-sets which are sorted and contain unique elements
//
// As a result we get many product terms (DNFs) which all would fullfill the coverage of the minterms
// We will select (the heuristic her is the shortes product terms) product terms and can build the minimum
// solution for the coverage. This is deterministic.
//
// So there are 2 minimum results: P1P4P5 and P2P3P6
//
// 1.:   Ac bC aB
// 2.:   Bc Ab aC
// Combined with the essential prime implicant AD we will get 2 equal cost minimum DNFs as solution of the coverage problem
//
//    1:   AD+Ac+aB+bC 
//    2:   AD+Ab+Bc+aC
//
//
// So the pronlem at hand is that we need to apply the distributive law, or simpler said, multiplying
// out a CNF, like (P1+P2)(P3+P4)(P1+P3)(P5+P6)(P2+P5)(P4+P6)
//
// As a first step, we eliminate the "P" and jsut use the indices
// Then we notice that multipliying out some (1+2)(3+4) is in the end just the concatenation of numbers
// or adding elements to a resulting set. So the resulting set is:    1,3 + 1,4 + 2,3 + 2,4
// By further analysis, we detect that some indices will be repeated in the terms (sets). If we
// multiply (1+2)(1+3) then we will get 1,1 + 1,3 + 2,1 + 2,3. In 1,1 one 1 is redundant. With the
// idempoence law we can eleminte the redundacy. We are using the STL::set for this purpose.
// This will allow only unique elements and serves our purpose well. Same approach will be used
// for identical product terms. They will also not be allowed by using a stl::set
//
// The whole complicated algroithm can so be implemented with only a very few lines of code.





#include "petrick.hpp"

#include <algorithm>

// Functor operator for applying Petricks methhod

ProductTermVector PetricksMethod::operator ()(const CNF& cnf)
{
	// We select an iterative approach. Start with the first Element of the CNF (which is a DNF)
	// And we sorte the result of each iterative operation again in this element
	DNF resultingDNF{ cnf[0] };
	// We will always start with the element 1 (not element 0) becuase in 0 is the initial value
	// or respectively the intermediate result
	for (CNF::size_type dnfInCnfIndex = 1; dnfInCnfIndex < cnf.size(); ++dnfInCnfIndex)
	{
		// Result of multipliyong out od the intermediate (initial) value with the current CNF Product term
		DNF intermediateCalculatedDNF;
		// Now go through all elements of the intermediate (initial) product term/DNF
		// For (1+2)(3+4)  this would be the (1+2) part
		for (const ProductTerm& productTermLeftSide : resultingDNF)
		{
			// Next we will iterate over all Minterms in the next DNF
			// For (1+2)(3+4)  this would be the (3+4) part
			for (const ProductTerm& productTermRightSide : cnf[dnfInCnfIndex])
			{
				ProductTerm productTerm{ productTermLeftSide }; // Resulting Product term is now 1
				// Add all elements from the right side
				productTerm.insert(productTermRightSide.begin(), productTermRightSide.end());	// Resulting Product term is now 1,2
				intermediateCalculatedDNF.insert(std::move(productTerm));  // Store this one
				// And continue to add more product terms. The stl::set will ensure the idempotence law and prevent memory waste
			}
		}
		// And now add all found terms to the result and continue with the next element of the right hand side
		// Please note: also here the set will prevent double terms
		resultingDNF = std::move(intermediateCalculatedDNF);
	}

	// Now we have the result (with 10 lines of code). The result contains all product terms in DNF
	// But for our prupose we are only interested in the minimum size terms
	// so, lets find the element with the minimu size (can be more than one)
	uint minLength{ narrow_cast<uint>(std::min_element(resultingDNF.begin(), resultingDNF.end(), [](const ProductTerm & left, const ProductTerm & right) noexcept {return left.size() < right.size(); })->size()) };
	// And from the big list of the DNF with all product terms, we copy all elements having the minimu size to the result. These are our best coverage sets
	ProductTermVector cheapestVector;
	// Copy result and return it to caller
	std::copy_if(resultingDNF.begin(), resultingDNF.end(), std::back_inserter(cheapestVector), [&minLength](const ProductTerm& pt) noexcept {return pt.size() == minLength; });
	return cheapestVector;
}

