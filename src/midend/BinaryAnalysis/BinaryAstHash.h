#ifndef ROSE_BinaryAnalysis_AstHash_H
#define ROSE_BinaryAnalysis_AstHash_H

#include <MemoryMap.h>
#include <Partitioner2/BasicBlock.h>
#include "Combinatorics.h"

namespace Rose {
namespace BinaryAnalysis {

     /**
      * @class AstHash
      *
      * A simple traversal that builds up a hash code for the AST.  
      * This hash was intended to uniquely identify a function, but it
      * can be used for any AST.
      * The hash can be generated by any hashing function in
      * Rose::Combinatorics::Hasher.
      * Also has options for what should be included in the hash. 
      * including constants, instruction addresses, mnemonics or byte
      * codes
      *
      * Use like this:
      *  AstHash astHash(hasher, false, false);
      *  astHash.traverse(node,preorder);
      *  hash = astHash.hasher().digest();
      *  
      **/
     class AstHash: public AstSimpleProcessing
        {
          public:
            // @brief Constructor just initializes output buffer  
            AstHash(Rose::Combinatorics::Hasher& in_hasher, bool includeConstants = false, bool includeAddresses = false) : 
                hasher_(in_hasher), includeConstants(includeConstants), includeAddresses(includeAddresses){}
               
            // @brief Generates the hash. Called by traverse
            void visit(SgNode* n);
            
            // @brief For the Partitioner Interface, traverses a basic block
            void appendBasicBlock(Rose::BinaryAnalysis::Partitioner2::BasicBlock::Ptr bb );

            // @brief Gets the Rose::Combinatorics::Hasher
            const Rose::Combinatorics::Hasher& hasher() const { return hasher_; }; 

        private:
            // @brief The reference used to the output data buffer,
            // get the opcodes
            Rose::Combinatorics::Hasher& hasher_;

            // @brief Include constant values in the hash, usually
            // false to avoid linking changing pointers, and therefore
            // the signature
            bool includeConstants;

            // @brief Include instruction addresses in the hash, usually
            // false to avoid different memory map changing the signature
            bool includeAddresses;

        };

} // namespace
} // namespace

#endif
