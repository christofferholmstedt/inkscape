#ifndef __NR_MATRIX_H__
#define __NR_MATRIX_H__

/** \file
 * Definition of NRMatrix and NR::Matrix types.
 *
 * \note Operator functions (e.g. Matrix * Matrix etc.) are mostly in
 * libnr/nr-matrix-ops.h.  See end of file for discussion.
 *
 * Main authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>:
 *     Original NRMatrix definition and related macros.
 *
 *   Nathan Hurst <njh@mail.csse.monash.edu.au>:
 *     NR::Matrix class version of the above.
 *
 * This code is in public domain.
 */

#include <glib/gmessages.h>

#include "libnr/nr-coord.h"
#include "libnr/nr-values.h"
#include <libnr/nr-rotate.h>
#include <libnr/nr-scale.h>
#include <libnr/nr-translate.h>

/// NRMatrix is the obsolete form of NR::Matrix.
/// It consists of six NR::Coord values.
struct NRMatrix {
    NR::Coord c[6];

    NR::Coord &operator[](int i) { return c[i]; }
    NR::Coord operator[](int i) const { return c[i]; }
};

#define nr_matrix_set_identity(m) (*(m) = NR_MATRIX_IDENTITY)

#define nr_matrix_test_identity(m,e) (!(m) || NR_MATRIX_DF_TEST_CLOSE(m, &NR_MATRIX_IDENTITY, e))

#define nr_matrix_test_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_CLOSE(m0, m1, e)))
#define nr_matrix_test_transform_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSFORM_CLOSE(m0, m1, e)))
#define nr_matrix_test_translate_equal(m0,m1,e) ((!(m0) && !(m1)) || ((m0) && (m1) && NR_MATRIX_DF_TEST_TRANSLATE_CLOSE(m0, m1, e)))

NRMatrix *nr_matrix_invert(NRMatrix *d, NRMatrix const *m);

/* d,m0,m1 needn't be distinct in any of these multiply routines. */

NRMatrix *nr_matrix_multiply(NRMatrix *d, NRMatrix const *m0, NRMatrix const *m1);

NRMatrix *nr_matrix_set_translate(NRMatrix *m, NR::Coord const x, NR::Coord const y);

NRMatrix *nr_matrix_set_scale(NRMatrix *m, NR::Coord const sx, NR::Coord const sy);

NRMatrix *nr_matrix_set_rotate(NRMatrix *m, NR::Coord const theta);

#define NR_MATRIX_DF_TRANSFORM_X(m,x,y) ((*(m))[0] * (x) + (*(m))[2] * (y) + (*(m))[4])
#define NR_MATRIX_DF_TRANSFORM_Y(m,x,y) ((*(m))[1] * (x) + (*(m))[3] * (y) + (*(m))[5])

#define NR_MATRIX_DF_EXPANSION2(m) (fabs((*(m))[0] * (*(m))[3] - (*(m))[1] * (*(m))[2]))
#define NR_MATRIX_DF_EXPANSION(m) (sqrt(NR_MATRIX_DF_EXPANSION2(m)))

namespace NR {

/**
 * The Matrix class.
 * 
 * For purposes of multiplication, points should be thought of as row vectors
 *
 *    p = ( p[X] p[Y]  1  )
 *
 * to be right-multiplied by transformation matrices
 * \verbatim
    c[] = | c[0] c[1]  0  |
          | c[2] c[3]  0  |
          | c[4] c[5]  1  |                           \endverbatim
 *
 * (so the columns of the matrix correspond to the columns (elements) of the result,
 * and the rows of the matrix correspond to columns (elements) of the "input").
 */
class Matrix {


    public:

    /**
     * Various forms of constructor
     */

    /**
     *
     */
    explicit Matrix() { }


    /**
     *
     */
    Matrix(Matrix const &m) {

        NR::Coord const *src = m._c;
        NR::Coord *dest      = _c;

        *dest++ = *src++;  //0
        *dest++ = *src++;  //1
        *dest++ = *src++;  //2
        *dest++ = *src++;  //3
        *dest++ = *src++;  //4
        *dest   = *src  ;  //5

    }




    /**
     *
     */
    Matrix(NRMatrix const &m) {

        NR::Coord const *src = m.c;
        NR::Coord *dest      = _c;

        *dest++ = *src++;  //0
        *dest++ = *src++;  //1
        *dest++ = *src++;  //2
        *dest++ = *src++;  //3
        *dest++ = *src++;  //4
        *dest   = *src  ;  //5

    }




    /**
     *
     */
    Matrix(double c0, double c1,
           double c2, double c3,
           double c4, double c5) {

        NR::Coord *dest = _c;

        *dest++ = c0;  //0
        *dest++ = c1;  //1
        *dest++ = c2;  //2
        *dest++ = c3;  //3
        *dest++ = c4;  //4
        *dest   = c5;  //5

    }



    /**
     *
     */
    Matrix &operator=(Matrix const &m) {

        NR::Coord const *src = m._c;
        NR::Coord *dest      = _c;

        *dest++ = *src++;  //0
        *dest++ = *src++;  //1
        *dest++ = *src++;  //2
        *dest++ = *src++;  //3
        *dest++ = *src++;  //4
        *dest   = *src  ;  //5

        return *this;
    }




    /**
     *
     */
    explicit Matrix(scale const &sm) {

        NR::Coord *dest  = _c;

        *dest++ = sm[X]; //0
        *dest++ = 0.0;   //1
        *dest++ = 0.0;   //2
        *dest++ = sm[Y]; //3
        *dest++ = 0.0;   //4
        *dest   = 0.0;   //5

    }






    /**
     *
     */
    explicit Matrix(rotate const &r) {

        NR::Coord *dest  = _c;

        *dest++ =  r.vec[X]; //0
        *dest++ =  r.vec[Y]; //1
        *dest++ = -r.vec[Y]; //2
        *dest++ =  r.vec[X]; //3
        *dest++ =  0.0;      //4
        *dest   =  0.0;      //5

    }




    /**
     *
     */
    explicit Matrix(translate const &tm) {

        NR::Coord *dest  = _c;

        *dest++ =  1.0;   //0
        *dest++ =  0.0;   //1
        *dest++ =  0.0;   //2
        *dest++ =  1.0;   //3
        *dest++ =  tm[X]; //4
        *dest   =  tm[Y]; //5
    }



    /**
     *
     */
    Matrix(NRMatrix const *nr);


    /**
     *
     */
    bool test_identity() const;


    /**
     *
     */
    bool is_translation(Coord const eps = 1e-6) const;


    /**
     *
     */
    Matrix inverse() const;


    /**
     *
     */
    Matrix &operator*=(Matrix const &other);


    /**
     *
     */
    Matrix &operator*=(scale const &other);



    /**
     *
     */
    Matrix &operator*=(translate const &other) {
        _c[4] += other[X];
        _c[5] += other[Y];
        return *this;
    }



    /**
     *
     */
    inline Coord &operator[](int const i) {
        return _c[i];
    }



    /**
     *
     */
    inline Coord operator[](int const i) const {
        return _c[i];
    }


    /**
     *
     */
    void set_identity();
	
    /**
     *
     */
    Coord det() const;


    /**
     *
     */
    Coord descrim2() const;


    /**
     *
     */
    Coord descrim() const;


    /**
     *
     */
    double expansion() const;


    /**
     *
     */
    double expansionX() const;


    /**
     *
     */
    double expansionY() const;
	
    // legacy


    /**
     *
     */
    Matrix &assign(Coord const *array);


    /**
     *
     */
    NRMatrix *copyto(NRMatrix* nrm) const;


    /**
     *
     */
    Coord *copyto(Coord *array) const;



    /**
     *
     */
    operator NRMatrix&() {
        g_assert(sizeof(_c) == sizeof(NRMatrix));
        return *reinterpret_cast<NRMatrix *>(_c);
    }



    /**
     *
     */
    operator NRMatrix const&() const {
        g_assert(sizeof(_c) == sizeof(NRMatrix));
        return *reinterpret_cast<const NRMatrix *>(_c);
    }



    /**
     *
     */
    operator NRMatrix*() {
        g_assert(sizeof(_c) == sizeof(NRMatrix));
        return reinterpret_cast<NRMatrix *>(_c);
    }


    /**
     *
     */
    operator NRMatrix const*() const {
        g_assert(sizeof(_c) == sizeof(NRMatrix));
        return reinterpret_cast<NRMatrix const *>(_c);
    }


    private:


    NR::Coord _c[6];
};

/** A function to print out the Matrix (for debugging) */
inline std::ostream &operator<< (std::ostream &out_file, const NR::Matrix &m) {
    out_file << "A: " << m[0] << "  C: " << m[2] << "  E: " << m[4] << "\n";
    out_file << "B: " << m[1] << "  D: " << m[3] << "  F: " << m[5] << "\n";
    return out_file;
}

extern void assert_close(Matrix const &a, Matrix const &b);

} /* namespace NR */







/** \note
 * Discussion of splitting up nr-matrix.h into lots of little files:
 *
 *   Advantages:
 *
 *    - Reducing amount of recompilation necessary when anything changes.
 *
 *    - Hopefully also reducing compilation time by reducing the number of inline
 *      function definitions encountered by the compiler for a given .o file.
 *      (No timing comparisons done yet.  On systems without much memory available
 *      for caching, this may be outweighed by additional I/O costs.)
 *
 *   Disadvantages:
 *
 *    - More #include lines necessary per file.  If a compile fails due to
 *      not having all the necessary #include lines, then the developer needs
 *      to spend some time working out what #include to add.
 */

#endif /* !__NR_MATRIX_H__ */


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
