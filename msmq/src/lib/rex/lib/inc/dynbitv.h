/****************************************************************************/
/*  File:       dynbitv.h                                                  */
/*  Author:     J. Kanze                                                    */
/*  Date:       06/01/1994                                                  */
/*      Copyright (c) 1994 James Kanze                                      */
/* ------------------------------------------------------------------------ */
/*  Modified:   11/11/1994  J. Kanze                                        */
/*      Merged in DynBitVect.  Adapted to new coding standards.          */
/*  Modified:   21/11/1994  J. Kanze                                        */
/*      Made interface conform to ArrayOf.                               */
/*  Modified:   07/08/2000  J. Kanze                                        */
/*      Ported to current library conventions and the standard library.     */
/* ------------------------------------------------------------------------ */
//      CRexDynBitVector:
//      ================
//
//      A dynamic bit vector is conceptually an infinitly long bit
//      vector, with indexes starting at 0.  In fact, even
//      conceptually, the length is limited by the range of the index
//      type, a CBitVectorImpl::BitIndex.
//
//      The bit vector can in fact be viewed as consisting of two
//      parts, an active part and a trailing part.  All of the bits in
//      the trailing part have the same value, and in fact are
//      represented by a single flag, which the user can read.  The
//      size of the active part increases dynamically; in the current
//      implementation, it will only decrease on assignment.  There
//      are special functions for the client to read the size of the
//      active part and the value of the trailing part.
//
//      isSet:              returns true if (any of) the specified
//                          bits are set.  Possible specifiers are:
//                          index (an int, selecting a specific bit),
//                          any, none, or all.
//
//      find:               find the first matching bit starting at a
//                          given point in the vector.
//
//      operator[]:         indexing.  An out of bounds index causes
//                          an assertion failure.
//
//      set:                set one or more bits, as selected by the
//                          argument.  The following argument types
//                          are allowed: index (an int, selecting a
//                          single bit), all, or another CBitVector
//                          (selecting all of the bits set in this
//                          set).  When another CBitVector is used,
//                          it must either be dynamic, or have exactly
//                          the same size as this bit vector.
//
//      reset:              as above, but resets the specified bit(s).
//
//      toggle:             as set, but toggles the specified bit(s).
//
//      intersect:          logical and, only supported with another
//                          CRexDynBitVector.
//
//      complement:         complement.  The equivalent of `toggle(
//                          all )'.
//
//      The functions and, or, xor and not have been removed from this
//      version, as their names conflicted with new keywords:-).
//
//      isEqual,
//      isNotEqual,
//      isSubsetOf,
//      isStrictSubsetOf,
//      isSupersetOf,
//      isStrictSupersetOf: returns true if the specified relation
//                          is true, false otherwise.  In all
//                          cases, the bit vectors are processed
//                          semantically as a `set of cardinal'.
//
//      operators:          in addition to [] and =, the following
//                          operators are defined: &, |, ^, ~, &=, |=,
//                          ^=, +, +=, *, *=, -, -=, ==, !=, <, <=, >,
//                          >=.  Operators + and += have the same
//                          semantics as | and |=, operators * and *=
//                          the same as & and &=.  The comparison
//                          operators are defined by processing the
//                          bit vectors semantically as `set of
//                          cardinal', with < as isStrictSubsetOf, <=
//                          isSubsetOf, > isStrictSupersetOf and >=
//                          isSupersetOf.
// --------------------------------------------------------------------------

#ifndef REX_DYNBITV_HH
#define REX_DYNBITV_HH

#include <inc/global.h>
#include <inc/bitvimpl.h>

// ==========================================================================
//      CRexDynBitVector:
//      ================
//
//      This is a (non-template) class for a conceptually infinite
//      length bit vector.
// --------------------------------------------------------------------------

class CRexDynBitVector : public CBitVectorImpl
{
    typedef CBitVectorImpl
                        Impl ;
    enum { granularity = sizeof( long ) * CHAR_BIT } ;
                                        // must be power of 2, >= number of
                                        // bits in Byte.
public :
    typedef CBitVIterator< CRexDynBitVector >
                        Iterator ;

    //      Constructors, Destructors and Assignment:
    //      =========================================
    //
    //      The following constructors (in addition to the copy
    //      constructor) are provided:
    //
    //      default:            Initializes all of the bits to 0.
    //
    //      unsigned long:      Uses the unsigned long to initialize
    //                          the bits in the vector; the low order
    //                          bit of the unsigned long goes into bit
    //                          0, and so on.  If the bit vector
    //                          contains more bits than an unsigned
    //                          long, the additional bits are
    //                          initialized to 0.  If the bit vector
    //                          contains less bits, the extra bits are
    //                          simply ignored.
    //
    //      std::string:        The string is used to initialize the
    //                          bits in the vector.  '0', 'F' or 'f'
    //                          initialize a bit to 0, '1', 'T' or 't'
    //                          to 1.  White space is ignored.  Any
    //                          other characters will result in an
    //                          assertion failure (to be replaced by
    //                          an exception ulteriorly).
    //
    //                          The first significant character in the
    //                          string initializes bit 0, the next bit
    //                          1, and so on.  WARNING: this is the
    //                          opposite of the proposed ISO 'bits'
    //                          class!
    //
    //                          If the number of significant
    //                          characters in the string is less than
    //                          the number of bits in the vector, the
    //                          additional bits will be initialized to
    //                          0.  It is an error (assertion failure
    //                          for now) for the string to contain
    //                          more significant characters than there
    //                          are bits.
    //
    //      const char*:        Exactly as std::string.
    // ----------------------------------------------------------------------
                        CRexDynBitVector() ;
    explicit            CRexDynBitVector( unsigned long initValue ) ;
    explicit            CRexDynBitVector( std::string const& initValue ) ;
    explicit            CRexDynBitVector( char const* initValue ) ;

    //      Access functions:
    //      =================
    //
    //      These functions are used to read the bit vector as a unit.
    //
    //      asShort or asLong will cause an assertion failure if any
    //      bit after the first 16 or 32 respectively is set.  (It was
    //      deemed preferable to use the fixed values of 16 and 32,
    //      rather than the actual number of bits in a short or a
    //      long, for portability reasons.)  The value returned is
    //      suitable for use as a parameter to the constructor.
    //
    //      'asString' returns a string consisting of uniquely 0's and
    //      1's, suitable for use in the constructor.  (Ie: bit 0 is
    //      the first character in the string, bit 1 the next, and so
    //      on.)  The length of the string is such that the last
    //      character will always be a '1'; trailing unset bits are
    //      not output.
    // ----------------------------------------------------------------------
    unsigned short      asShort() const ;
    unsigned long       asLong() const ;
    std::string      asString() const ;

    //      Predicates:
    //      ===========
    //
    //      These const Functions return true or false.
    //
    //      isSet:      Returns true if the given bit is set.
    //
    //      contains:   An alias for isSet.  Semantically, it refers to
    //                  the set nature of the bit vector,
    //
    //      isEmpty:    Returns true if no bits are set.  (Set is empty.)
    // ----------------------------------------------------------------------
    bool                isSet( BitIndex bitNo ) const ;
    bool                contains( BitIndex bitNo ) const ;
    bool                isEmpty() const ;

    //      Attributes:
    //      ===========
    //
    //      find:       returns the index of the first bit matching
    //                  the targetValue, starting from the 'from'
    //                  parameter (inclusive).  If no bit matches,
    //                  returns CBitVectorImpl::infinity.
    //
    //      count:      returns the number of *set* bits in the
    //                  vector.
    //
    //      activeLength:   returns the number of bits in the active
    //                      part.  It is guarenteed that all bits
    //                      beyond this have the same value.
    //
    //      trailingValue:  returns the value of the bits in the
    //                      active part.
    //
    //      Note that with judicious use of activeLength and
    //      trailingValue, it is possible to "iterate" over the entire
    //      vector, despite its "infinite" length.
    // ----------------------------------------------------------------------
    BitIndex            find( bool targetValue , BitIndex from = 0 ) const ;
    BitIndex            count() const ;
    BitIndex            activeLength() const ;
    bool                trailingValue() const ;

    Iterator            iterator() const ;
    Iterator            begin() const ;
    Iterator            end() const ;

    //      Relationships:
    //      ==============
    //
    //      A partial ordering is defined by the subset relationship.
    // ----------------------------------------------------------------------
    bool                isEqual( CRexDynBitVector const& other ) const ;
    bool                isNotEqual( CRexDynBitVector const& other ) const ;
    bool                isSubsetOf( CRexDynBitVector const& other ) const ;
    bool                isStrictSubsetOf(
                            CRexDynBitVector const& other ) const ;
    bool                isSupersetOf( CRexDynBitVector const& other ) const ;
    bool                isStrictSupersetOf(
                            CRexDynBitVector const& other ) const ;

    //      Basic bit manipulations:
    //      ========================
    //
    //      Set sets the bit(s) to 1 (logical or).
    //
    //      Reset resets the bit(s) to 0.
    //
    //      Complement toggles (complements) the bit(s) (logical xor).
    //
    //      Intersect is a logical and.
    //
    //      With the exception of intersect, all of these functions
    //      have three variants: one for a single bit, one where a
    //      second array specifies the bits concerned, and one with
    //      the parameter "all", which acts on all of the bits.
    //
    //      These functions provide the basis for the logical
    //      operations defined later.
    // ----------------------------------------------------------------------
    void                set( BitIndex bitNo ) ;
    void                set( CRexDynBitVector const& other ) ;
    void                set() ;

    void                reset( BitIndex bitNo ) ;
    void                reset( CRexDynBitVector const& other ) ;
    void                reset() ;
    void                clear() ;       // synonyme for reset...

    void                complement( BitIndex bitNo ) ;
    void                complement( CRexDynBitVector const& other ) ;
    void                complement() ;

    void                intersect( CRexDynBitVector const& other ) ;

    //      Operator overloads:
    //      ===================
    //
    //      All reasonable operators are overloaded.
    //
    //      Note that even the classic binary operators are overloaded
    //      as member functions.  This is necessary, as most compilers
    //      will not allow a template function with a non-type
    //      argument (ie: bitCnt).
    //
    //      The operators for comparison of inequality (<, <=, >, >=)
    //      use the subset relationship, ie: < means is a strict
    //      subset of, etc.  Since this relation defines only a
    //      partial ordering, '!  (a < b)' does not necessarily imply
    //      'a >= b'.
    // ----------------------------------------------------------------------
    CRexDynBitVector&    operator&=( CRexDynBitVector const& other ) ;
    CRexDynBitVector&    operator|=( CRexDynBitVector const& other ) ;
    CRexDynBitVector&    operator^=( CRexDynBitVector const& other ) ;

    CRexDynBitVector&    operator+=( CRexDynBitVector const& other ) ;
    CRexDynBitVector&    operator+=( BitIndex bitNo ) ;
    CRexDynBitVector&    operator-=( CRexDynBitVector const& other ) ;
    CRexDynBitVector&    operator-=( BitIndex bitNo ) ;
    CRexDynBitVector&    operator*=( CRexDynBitVector const& other ) ;

    bool                operator[]( BitIndex index ) const ;
    CBitVAccessProxy< CRexDynBitVector >
                        operator[]( BitIndex index ) ;

    //      Support functions:
    //      ==================
    //
    //      The following two functions are for support for container
    //      classes.
    //
    //      The compare functions defines an *arbitrary* ordering
    //      relationship, in no way related to subsets, for example.
    //      (It is not even guaranteed that the ordering will remain
    //      the same between releases.)
    // ----------------------------------------------------------------------
    unsigned            hash() const ;
    int                 compare( CRexDynBitVector const& other ) const ;

private :
    std::vector< Byte > myBuffer ;
    bool                myTrailingPart ;

    static BitIndex     byteCount( BitIndex bitCount ) ;
    BitIndex            bitCount() const ;
    Byte*               buffer() ;
    Byte const*         buffer() const ;
    void                init( std::string const& initValue ) ;
    void                enlargeActivePart( BitIndex minBitCount ) ;
} ;

#include <inc/dynbitv.inl>

inline bool
operator==( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isEqual( op2 ) ;
}

inline bool
operator!=( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isNotEqual( op2 ) ;
}

inline bool
operator<( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isStrictSubsetOf( op2 ) ;
}

inline bool
operator<=( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isSubsetOf( op2 ) ;
}

inline bool
operator>( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isStrictSupersetOf( op2 ) ;
}

inline bool
operator>=( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    return op1.isSupersetOf( op2 ) ;
}

inline CRexDynBitVector
operator+( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result += op2 ;
    return result ;
}

inline CRexDynBitVector
operator+( CRexDynBitVector const& op1 , CRexDynBitVector::BitIndex op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result += op2 ;
    return result ;
}

inline CRexDynBitVector
operator-( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result -= op2 ;
    return result ;
}

inline CRexDynBitVector
operator-( CRexDynBitVector const& op1 , CRexDynBitVector::BitIndex op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result -= op2 ;
    return result ;
}

inline CRexDynBitVector
operator*( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result *= op2 ;
    return result ;
}

inline CRexDynBitVector
operator|( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result |= op2 ;
    return result ;
}

inline CRexDynBitVector
operator&( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result &= op2 ;
    return result ;
}

inline CRexDynBitVector
operator^( CRexDynBitVector const& op1 , CRexDynBitVector const& op2 )
{
    CRexDynBitVector     result( op1 ) ;
    result ^= op2 ;
    return result ;
}

inline CRexDynBitVector
operator~( CRexDynBitVector const& op )
{
    CRexDynBitVector     result( op ) ;
    result.complement() ;
    return result ;
}
#endif
//  Local Variables:    --- for emacs
//  mode: c++           --- for emacs
//  tab-width: 8        --- for emacs
//  End:                --- for emacs
