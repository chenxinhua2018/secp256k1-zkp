#ifndef _SECP256K1_
# define _SECP256K1_

# ifdef __cplusplus
extern "C" {
# endif

#include <stdint.h>
#include <stddef.h>

# if !defined(SECP256K1_GNUC_PREREQ)
#  if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#   define SECP256K1_GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
#  else
#   define SECP256K1_GNUC_PREREQ(_maj,_min) 0
#  endif
# endif

# if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) )
#  if SECP256K1_GNUC_PREREQ(2,7)
#   define SECP256K1_INLINE __inline__
#  elif (defined(_MSC_VER))
#   define SECP256K1_INLINE __inline
#  else
#   define SECP256K1_INLINE
#  endif
# else
#  define SECP256K1_INLINE inline
# endif

/**Warning attributes
  * NONNULL is not used if SECP256K1_BUILD is set to avoid the compiler optimizing out
  * some paranoid null checks. */
# if defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
# else
#  define SECP256K1_WARN_UNUSED_RESULT
# endif
# if !defined(SECP256K1_BUILD) && defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_ARG_NONNULL(_x)  __attribute__ ((__nonnull__(_x)))
# else
#  define SECP256K1_ARG_NONNULL(_x)
# endif

/** Opaque data structure that holds context information (precomputed tables etc.).
 *  Only functions that take a pointer to a non-const context require exclusive
 *  access to it. Multiple functions that take a pointer to a const context may
 *  run simultaneously.
 */
typedef struct secp256k1_context_struct secp256k1_context_t;

/** Flags to pass to secp256k1_context_create. */
# define SECP256K1_CONTEXT_VERIFY (1 << 0)
# define SECP256K1_CONTEXT_SIGN   (1 << 1)
# define SECP256K1_CONTEXT_COMMIT (1 << 7)
# define SECP256K1_CONTEXT_RANGEPROOF (1 << 8)

/** Create a secp256k1 context object.
 *  Returns: a newly created context object.
 *  In:      flags: which parts of the context to initialize.
 */
secp256k1_context_t* secp256k1_context_create(
  int flags
) SECP256K1_WARN_UNUSED_RESULT;

/** Copies a secp256k1 context object.
 *  Returns: a newly created context object.
 *  In:      ctx: an existing context to copy
 */
secp256k1_context_t* secp256k1_context_clone(
  const secp256k1_context_t* ctx
) SECP256K1_WARN_UNUSED_RESULT;

/** Destroy a secp256k1 context object.
 *  The context pointer may not be used afterwards.
 */
void secp256k1_context_destroy(
  secp256k1_context_t* ctx
) SECP256K1_ARG_NONNULL(1);

/** Verify an ECDSA signature.
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *          -1: invalid public key
 *          -2: invalid signature
 * In:       ctx:       a secp256k1 context object, initialized for verification.
 *           msg32:     the 32-byte message hash being verified (cannot be NULL)
 *           sig:       the signature being verified (cannot be NULL)
 *           siglen:    the length of the signature
 *           pubkey:    the public key to verify with (cannot be NULL)
 *           pubkeylen: the length of pubkey
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const unsigned char *sig,
  int siglen,
  const unsigned char *pubkey,
  int pubkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

/** A pointer to a function to deterministically generate a nonce.
 * Returns: 1 if a nonce was successfully generated. 0 will cause signing to fail.
 * In:      msg32:     the 32-byte message hash being verified (will not be NULL)
 *          key32:     pointer to a 32-byte secret key (will not be NULL)
 *          attempt:   how many iterations we have tried to find a nonce.
 *                     This will almost always be 0, but different attempt values
 *                     are required to result in a different nonce.
 *          data:      Arbitrary data pointer that is passed through.
 * Out:     nonce32:   pointer to a 32-byte array to be filled by the function.
 * Except for test cases, this function should compute some cryptographic hash of
 * the message, the key and the attempt.
 */
typedef int (*secp256k1_nonce_function_t)(
  unsigned char *nonce32,
  const unsigned char *msg32,
  const unsigned char *key32,
  unsigned int attempt,
  const void *data
);

/** An implementation of RFC6979 (using HMAC-SHA256) as nonce generation function.
 * If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 * extra entropy.
 */
extern const secp256k1_nonce_function_t secp256k1_nonce_function_rfc6979;

/** A default safe nonce generation function (currently equal to secp256k1_nonce_function_rfc6979). */
extern const secp256k1_nonce_function_t secp256k1_nonce_function_default;


/** Create an ECDSA signature.
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, the private key was invalid, or there is not
 *              enough space in the signature (as indicated by siglen).
 *  In:      ctx:    pointer to a context object, initialized for signing (cannot be NULL)
 *           msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *  Out:     sig:    pointer to an array where the signature will be placed (cannot be NULL)
 *  In/Out:  siglen: pointer to an int with the length of sig, which will be updated
 *                   to contain the actual signature length (<=72).
 *
 * The sig always has an s value in the lower half of the range (From 0x1
 * to 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0,
 * inclusive), unlike many other implementations.
 * With ECDSA a third-party can can forge a second distinct signature
 * of the same message given a single initial signature without knowing
 * the key by setting s to its additive inverse mod-order, 'flipping' the
 * sign of the random point R which is not included in the signature.
 * Since the forgery is of the same message this isn't universally
 * problematic, but in systems where message malleability or uniqueness
 * of signatures is important this can cause issues.  This forgery can be
 * blocked by all verifiers forcing signers to use a canonical form. The
 * lower-S form reduces the size of signatures slightly on average when
 * variable length encodings (such as DER) are used and is cheap to
 * verify, making it a good choice. Security of always using lower-S is
 * assured because anyone can trivially modify a signature after the
 * fact to enforce this property.  Adjusting it inside the signing
 * function avoids the need to re-serialize or have curve specific
 * constants outside of the library.  By always using a canonical form
 * even in applications where it isn't needed it becomes possible to
 * impose a requirement later if a need is discovered.
 * No other forms of ECDSA malleability are known and none seem likely,
 * but there is no formal proof that ECDSA, even with this additional
 * restriction, is free of other malleability.  Commonly used serialization
 * schemes will also accept various non-unique encodings, so care should
 * be taken when this property is required for an application.
 */
int secp256k1_ecdsa_sign(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  unsigned char *sig,
  int *siglen,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Create a compact ECDSA signature (64 byte + recovery id).
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the secret key was invalid.
 *  In:      ctx:    pointer to a context object, initialized for signing (cannot be NULL)
 *           msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *  Out:     sig:    pointer to a 64-byte array where the signature will be placed (cannot be NULL)
 *                   In case 0 is returned, the returned signature length will be zero.
 *           recid:  pointer to an int, which will be updated to contain the recovery id (can be NULL)
 */
int secp256k1_ecdsa_sign_compact(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  unsigned char *sig64,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata,
  int *recid
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Recover an ECDSA public key from a compact signature.
 *  Returns: 1: public key successfully recovered (which guarantees a correct signature).
 *           0: otherwise.
 *  In:      ctx:        pointer to a context object, initialized for verification (cannot be NULL)
 *           msg32:      the 32-byte message hash assumed to be signed (cannot be NULL)
 *           sig64:      signature as 64 byte array (cannot be NULL)
 *           compressed: whether to recover a compressed or uncompressed pubkey
 *           recid:      the recovery id (0-3, as returned by ecdsa_sign_compact)
 *  Out:     pubkey:     pointer to a 33 or 65 byte array to put the pubkey (cannot be NULL)
 *           pubkeylen:  pointer to an int that will contain the pubkey length (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_recover_compact(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const unsigned char *sig64,
  unsigned char *pubkey,
  int *pubkeylen,
  int compressed,
  int recid
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);


/** Do an ellitic curve scalar multiplication in constant time.
 *  Returns: 1: exponentiation was successful
 *          -1: scalar was zero (cannot serialize output point)
 *          -2: scalar overflow
 *          -3: invalid input point
 *  In:      scalar:   a 32-byte scalar with which to multiple the point
 *  In/Out:  point:    pointer to 33 or 65 byte array containing an EC point
 *                     which will be updated in place
 *           pointlen: length of the point array, which will be updated by
 *                     the multiplication
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_point_multiply(
  unsigned char *point,
  int *pointlen,
  const unsigned char *scalar
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Verify an ECDSA secret key.
 *  Returns: 1: secret key is valid
 *           0: secret key is invalid
 *  In:      ctx: pointer to a context object (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Just validate a public key.
 *  Returns: 1: public key is valid
 *           0: public key is invalid
 *  In:      ctx:       pointer to a context object (cannot be NULL)
 *           pubkey:    pointer to a 33-byte or 65-byte public key (cannot be NULL).
 *           pubkeylen: length of pubkey
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *pubkey,
  int pubkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Compute the public key for a secret key.
 *  In:     ctx:        pointer to a context object, initialized for signing (cannot be NULL)
 *          compressed: whether the computed public key should be compressed
 *          seckey:     pointer to a 32-byte private key (cannot be NULL)
 *  Out:    pubkey:     pointer to a 33-byte (if compressed) or 65-byte (if uncompressed)
 *                      area to store the public key (cannot be NULL)
 *          pubkeylen:  pointer to int that will be updated to contains the pubkey's
 *                      length (cannot be NULL)
 *  Returns: 1: secret was valid, public key stores
 *           0: secret was invalid, try again
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_create(
  const secp256k1_context_t* ctx,
  unsigned char *pubkey,
  int *pubkeylen,
  const unsigned char *seckey,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Decompress a public key.
 * In:     ctx:       pointer to a context object (cannot be NULL)
 * In/Out: pubkey:    pointer to a 65-byte array to put the decompressed public key.
 *                    It must contain a 33-byte or 65-byte public key already (cannot be NULL)
 *         pubkeylen: pointer to the size of the public key pointed to by pubkey (cannot be NULL)
 *                    It will be updated to reflect the new size.
 * Returns: 0: pubkey was invalid
 *          1: pubkey was valid, and was replaced with its decompressed version
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_decompress(
  const secp256k1_context_t* ctx,
  unsigned char *pubkey,
  int *pubkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Export a private key in DER format.
 * In: ctx: pointer to a context object, initialized for signing (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_export(
  const secp256k1_context_t* ctx,
  const unsigned char *seckey,
  unsigned char *privkey,
  int *privkeylen,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Import a private key in DER format. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_import(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *privkey,
  int privkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a private key by adding tweak to it. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_add(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a public key by adding tweak times the generator to it.
 * In: ctx: pointer to a context object, initialized for verification (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_add(
  const secp256k1_context_t* ctx,
  unsigned char *pubkey,
  int pubkeylen,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** Tweak a private key by multiplying it with tweak. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_mul(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a public key by multiplying it with tweak.
 * In: ctx: pointer to a context object, initialized for verification (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_mul(
  const secp256k1_context_t* ctx,
  unsigned char *pubkey,
  int pubkeylen,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** Updates the context randomization.
 *  Returns: 1: randomization successfully updated
 *           0: error
 *  In:      ctx:       pointer to a context object (cannot be NULL)
 *           seed32:    pointer to a 32-byte random seed (NULL resets to initial state)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_context_randomize(
  secp256k1_context_t* ctx,
  const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1);


/** Generate a pedersen commitment.
 *  Returns 1: commitment successfully created.
 *          0: error
 *  In:     ctx:        pointer to a context object, initialized for signing and commitment (cannot be NULL)
 *          blind:      pointer to a 32-byte blinding factor (cannot be NULL)
 *          value:      unsigned 64-bit integer value to commit to.
 *  Out:    commit:     pointer to a 33-byte array for the commitment (cannot be NULL)
 *
 *  Blinding factors can be generated and verified in the same way as secp256k1 private keys for ECDSA.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_commit(
  const secp256k1_context_t* ctx,
  unsigned char *commit,
  unsigned char *blind,
  uint64_t value
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Computes the sum of multiple positive and negative blinding factors.
 *  Returns 1: sum successfully computed.
 *          0: error
 *  In:     ctx:        pointer to a context object (cannot be NULL)
 *          blinds:     pointer to pointers to 32-byte character arrays for blinding factors. (cannot be NULL)
 *          n:          number of factors pointed to by blinds.
 *          nneg:       how many of the initial factors should be treated with a positive sign.
 *  Out:    blind_out:  pointer to a 32-byte array for the sum (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_blind_sum(
  const secp256k1_context_t* ctx,
  unsigned char *blind_out,
  const unsigned char * const *blinds,
  int n,
  int npositive
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Verify a tally of pedersen commitments
 * Returns 1: commitments successfully sum to zero.
 *         0: Commitments do not sum to zero or other error.
 * In:     ctx:        pointer to a context object, initialized for commitment (cannot be NULL)
 *         commits:    pointer to pointers to 33-byte character arrays for the commitments. (cannot be NULL if pcnt is non-zero)
 *         pcnt:       number of commitments pointed to by commits.
 *         ncommits:   pointer to pointers to 33-byte character arrays for negative commitments. (cannot be NULL if ncnt is non-zero)
 *         ncnt:       number of commitments pointed to by ncommits.
 *         excess:     signed 64bit amount to add to the total to bring it to zero, can be negative.
 *
 * This computes sum(commit[0..pcnt)) - sum(ncommit[0..ncnt)) - excess*H == 0.
 *
 * A pedersen commitment is xG + vH where G and H are generators for the secp256k1 group and x is a blinding factor,
 * while v is the committed value. For a collection of commitments to sum to zero both their blinding factors and
 * values must sum to zero.
 *
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_verify_tally(
  const secp256k1_context_t* ctx,
  const unsigned char * const *commits,
  int pcnt,
  const unsigned char * const *ncommits,
  int ncnt,
  int64_t excess
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** Verify a proof that a committed value is within a range.
 * Returns 1: Value is within the range [0..2^64), the specifically proven range is in the min/max value outputs.
 *         0: Proof failed or other error.
 * In:   ctx: pointer to a context object, initialized for range-proof and commitment (cannot be NULL)
 *       commit: the 33-byte commitment being proved. (cannot be NULL)
 *       proof: pointer to character array with the proof. (cannot be NULL)
 *       plen: length of proof in bytes.
 * Out:  min_value: pointer to a unsigned int64 which will be updated with the minimum value that commit could have. (cannot be NULL)
 *       max_value: pointer to a unsigned int64 which will be updated with the maximum value that commit could have. (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_rangeproof_verify(
 const secp256k1_context_t* ctx,
 uint64_t *min_value,
 uint64_t *max_value,
 const unsigned char *commit,
 const unsigned char *proof,
 int plen
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Verify a range proof proof and rewind the proof to recover information sent by its author.
 *  Returns 1: Value is within the range [0..2^64), the specifically proven range is in the min/max value outputs, and the value and blinding were recovered.
 *          0: Proof failed, rewind failed, or other error.
 *  In:   ctx: pointer to a context object, initialized for range-proof and commitment (cannot be NULL)
 *        commit: the 33-byte commitment being proved. (cannot be NULL)
 *        proof: pointer to character array with the proof. (cannot be NULL)
 *        plen: length of proof in bytes.
 *        nonce: 32-byte secret nonce used by the prover (cannot be NULL)
 *  In/Out: blind_out: storage for the 32-byte blinding factor used for the commitment
 *        value_out: pointer to an unsigned int64 which has the exact value of the commitment.
 *        message_out: pointer to a 4096 byte character array to receive message data from the proof author.
 *        outlen:  length of message data written to message_out.
 *        min_value: pointer to an unsigned int64 which will be updated with the minimum value that commit could have. (cannot be NULL)
 *        max_value: pointer to an unsigned int64 which will be updated with the maximum value that commit could have. (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_rangeproof_rewind(
 const secp256k1_context_t* ctx,
 unsigned char *blind_out,
 uint64_t *value_out,
 unsigned char *message_out,
 int *outlen,
 const unsigned char *nonce,
 uint64_t *min_value,
 uint64_t *max_value,
 const unsigned char *commit,
 const unsigned char *proof,
 int plen
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8) SECP256K1_ARG_NONNULL(9) SECP256K1_ARG_NONNULL(10);

/** Author a proof that a committed value is within a range.
 *  Returns 1: Proof successfully created.
 *          0: Error
 *  In:     ctx:    pointer to a context object, initialized for range-proof, signing, and commitment (cannot be NULL)
 *          proof:  pointer to array to receive the proof, can be up to 5134 bytes. (cannot be NULL)
 *          min_value: constructs a proof where the verifer can tell the minimum value is at least the specified amount.
 *          commit: 33-byte array with the commitment being proved.
 *          blind:  32-byte blinding factor used by commit.
 *          nonce:  32-byte secret nonce used to initialize the proof (value can be reverse-engineered out of the proof if this secret is known.)
 *          exp:    Base-10 exponent. Digits below above will be made public, but the proof will be made smaller. Allowed range is -1 to 18.
 *                  (-1 is a special case that makes the value public. 0 is the most private.)
 *          min_bits: Number of bits of the value to keep private. (0 = auto/minimal, - 64).
 *          value:  Actual value of the commitment.
 *  In/out: plen:   point to an integer with the size of the proof buffer and the size of the constructed proof.
 *
 *  If min_value or exp is non-zero then the value must be on the range [0, 2^63) to prevent the proof range from spanning past 2^64.
 *
 *  If exp is -1 the value is revealed by the proof (e.g. it proves that the proof is a blinding of a specific value, without revealing the blinding key.)
 *
 *  This can randomly fail with probability around one in 2^100. If this happens, buy a lottery ticket and retry with a different nonce or blinding.
 *
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_rangeproof_sign(
 const secp256k1_context_t* ctx,
 unsigned char *proof,
 int *plen,
 uint64_t min_value,
 const unsigned char *commit,
 const unsigned char *blind,
 const unsigned char *nonce,
 int exp,
 int min_bits,
 uint64_t value
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6) SECP256K1_ARG_NONNULL(7);

/** Extract some basic information from a range-proof.
 *  Returns 1: Information successfully extracted.
 *          0: Decode failed.
 *  In:   ctx: pointer to a context object
 *        proof: pointer to character array with the proof.
 *        plen: length of proof in bytes.
 *  Out:  exp: Exponent used in the proof (-1 means the value isn't private).
 *        mantissa: Number of bits covered by the proof.
 *        min_value: pointer to an unsigned int64 which will be updated with the minimum value that commit could have. (cannot be NULL)
 *        max_value: pointer to an unsigned int64 which will be updated with the maximum value that commit could have. (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_rangeproof_info(
 const secp256k1_context_t* ctx,
 int *exp,
 int *mantissa,
 uint64_t *min_value,
 uint64_t *max_value,
 const unsigned char *proof,
 int plen
)SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/*************************************** ex *****************************/

typedef secp256k1_context_t secp256k1_context;

typedef struct {
    unsigned char data[64];
} secp256k1_pubkey;

typedef secp256k1_nonce_function_t secp256k1_nonce_function;

/** All flags' lower 8 bits indicate what they're for. Do not use directly. */
#define SECP256K1_FLAGS_TYPE_MASK ((1 << 8) - 1)
#define SECP256K1_FLAGS_TYPE_CONTEXT (1 << 0)
#define SECP256K1_FLAGS_TYPE_COMPRESSION (1 << 1)
/** The higher bits contain the actual data. Do not use directly. */
// #define SECP256K1_FLAGS_BIT_CONTEXT_VERIFY (1 << 8)
// #define SECP256K1_FLAGS_BIT_CONTEXT_SIGN (1 << 9)
#define SECP256K1_FLAGS_BIT_COMPRESSION (1 << 8)

/** Flags to pass to secp256k1_context_create. */
// #define SECP256K1_CONTEXT_VERIFY (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_VERIFY)
// #define SECP256K1_CONTEXT_SIGN (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_SIGN)
#define SECP256K1_CONTEXT_NONE (SECP256K1_FLAGS_TYPE_CONTEXT)

/** Flag to pass to secp256k1_ec_pubkey_serialize and secp256k1_ec_privkey_export. */
#define SECP256K1_EC_COMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION | SECP256K1_FLAGS_BIT_COMPRESSION)
#define SECP256K1_EC_UNCOMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION)

/** Opaque data structured that holds a parsed ECDSA signature.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, or
 *  comparison, use the secp256k1_ecdsa_signature_serialize_* and
 *  secp256k1_ecdsa_signature_parse_* functions.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_ecdsa_signature;

/** Serialize an ECDSA signature in compact (64 byte) format.
 *
 *  Returns: 1
 *  Args:   ctx:       a secp256k1 context object
 *  Out:    output64:  a pointer to a 64-byte array to store the compact serialization
 *  In:     sig:       a pointer to an initialized signature object
 *
 *  See secp256k1_ecdsa_signature_parse_compact for details about the encoding.
 */
// SECP256K1_WARN_UNUSED_RESULT
int secp256k1_ecdsa_signature_serialize_compact(
    const secp256k1_context* ctx,
    unsigned char *output64,
    const secp256k1_ecdsa_signature* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse an ECDSA signature in compact (64 bytes) format.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args: ctx:      a secp256k1 context object
 *  Out:  sig:      a pointer to a signature object
 *  In:   input64:  a pointer to the 64-byte array to parse
 *
 *  The signature must consist of a 32-byte big endian R value, followed by a
 *  32-byte big endian S value. If R or S fall outside of [0..order-1], the
 *  encoding is invalid. R and S with value 0 are allowed in the encoding.
 *
 *  After the call, sig will always be initialized. If parsing failed or R or
 *  S are zero, the resulting sig value is guaranteed to fail validation for any
 *  message and public key.
 */
int secp256k1_ecdsa_signature_parse_compact(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature* sig,
    const unsigned char *input64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Compute the public key for a secret key.
 *
 *  Returns: 1: secret was valid, public key stores
 *           0: secret was invalid, try again
 *  Args:   ctx:        pointer to a context object, initialized for signing (cannot be NULL)
 *  Out:    pubkey:     pointer to the created public key (cannot be NULL)
 *  In:     seckey:     pointer to a 32-byte private key (cannot be NULL)
 */
int secp256k1_ec_pubkey_create_ex(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);


/** Create an ECDSA signature.
 *
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the private key was invalid.
 *  Args:    ctx:    pointer to a context object, initialized for signing (cannot be NULL)
 *  Out:     sig:    pointer to an array where the signature will be placed (cannot be NULL)
 *  In:      msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *
 * The created signature is always in lower-S form. See
 * secp256k1_ecdsa_signature_normalize for more details.
 */
int secp256k1_ecdsa_sign_ex(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature *sig,
    const unsigned char *msg32,
    const unsigned char *seckey,
    secp256k1_nonce_function noncefp,
    const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Parse a variable-length public key into the pubkey object.
 *
 *  Returns: 1 if the public key was fully valid.
 *           0 if the public key could not be parsed or is invalid.
 *  Args: ctx:      a secp256k1 context object.
 *  Out:  pubkey:   pointer to a pubkey object. If 1 is returned, it is set to a
 *                  parsed version of input. If not, its value is undefined.
 *  In:   input:    pointer to a serialized public key
 *        inputlen: length of the array pointed to by input
 *
 *  This function supports parsing compressed (33 bytes, header byte 0x02 or
 *  0x03), uncompressed (65 bytes, header byte 0x04), or hybrid (65 bytes, header
 *  byte 0x06 or 0x07) format public keys.
 */
int secp256k1_ec_pubkey_parse(
    const secp256k1_context* ctx,
    secp256k1_pubkey* pubkey,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);


/** Serialize a pubkey object into a serialized byte sequence.
 *
 *  Returns: 1 always.
 *  Args:   ctx:        a secp256k1 context object.
 *  Out:    output:     a pointer to a 65-byte (if compressed==0) or 33-byte (if
 *                      compressed==1) byte array to place the serialized key
 *                      in.
 *  In/Out: outputlen:  a pointer to an integer which is initially set to the
 *                      size of output, and is overwritten with the written
 *                      size.
 *  In:     pubkey:     a pointer to a secp256k1_pubkey containing an
 *                      initialized public key.
 *          flags:      SECP256K1_EC_COMPRESSED if serialization should be in
 *                      compressed format, otherwise SECP256K1_EC_UNCOMPRESSED.
 */
// SECP256K1_WARN_UNUSED_RESULT
int secp256k1_ec_pubkey_serialize(
    const secp256k1_context* ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_pubkey* pubkey,
    unsigned int flags
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);


/** Serialize an ECDSA signature in DER format.
 *
 *  Returns: 1 if enough space was available to serialize, 0 otherwise
 *  Args:   ctx:       a secp256k1 context object
 *  Out:    output:    a pointer to an array to store the DER serialization
 *  In/Out: outputlen: a pointer to a length integer. Initially, this integer
 *                     should be set to the length of output. After the call
 *                     it will be set to the length of the serialization (even
 *                     if 0 was returned).
 *  In:     sig:       a pointer to an initialized signature object
 */
int secp256k1_ecdsa_signature_serialize_der(
    const secp256k1_context* ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_ecdsa_signature* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Convert a signature to a normalized lower-S form.
 *
 *  Returns: 1 if sigin was not normalized, 0 if it already was.
 *  Args: ctx:    a secp256k1 context object
 *  Out:  sigout: a pointer to a signature to fill with the normalized form,
 *                or copy if the input was already normalized. (can be NULL if
 *                you're only interested in whether the input was already
 *                normalized).
 *  In:   sigin:  a pointer to a signature to check/normalize (cannot be NULL,
 *                can be identical to sigout)
 *
 *  With ECDSA a third-party can forge a second distinct signature of the same
 *  message, given a single initial signature, but without knowing the key. This
 *  is done by negating the S value modulo the order of the curve, 'flipping'
 *  the sign of the random point R which is not included in the signature.
 *
 *  Forgery of the same message isn't universally problematic, but in systems
 *  where message malleability or uniqueness of signatures is important this can
 *  cause issues. This forgery can be blocked by all verifiers forcing signers
 *  to use a normalized form.
 *
 *  The lower-S form reduces the size of signatures slightly on average when
 *  variable length encodings (such as DER) are used and is cheap to verify,
 *  making it a good choice. Security of always using lower-S is assured because
 *  anyone can trivially modify a signature after the fact to enforce this
 *  property anyway.
 *
 *  The lower S value is always between 0x1 and
 *  0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0,
 *  inclusive.
 *
 *  No other forms of ECDSA malleability are known and none seem likely, but
 *  there is no formal proof that ECDSA, even with this additional restriction,
 *  is free of other malleability. Commonly used serialization schemes will also
 *  accept various non-unique encodings, so care should be taken when this
 *  property is required for an application.
 *
 *  The secp256k1_ecdsa_sign function will by default create signatures in the
 *  lower-S form, and secp256k1_ecdsa_verify will not accept others. In case
 *  signatures come from a system that cannot enforce this property,
 *  secp256k1_ecdsa_signature_normalize must be called before verification.
 */
int secp256k1_ecdsa_signature_normalize(
    const secp256k1_context* ctx,
    secp256k1_ecdsa_signature *sigout,
    const secp256k1_ecdsa_signature *sigin
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);


/** Verify an ECDSA signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect or unparseable signature
 *  Args:    ctx:       a secp256k1 context object, initialized for verification.
 *  In:      sig:       the signature being verified (cannot be NULL)
 *           msg32:     the 32-byte message hash being verified (cannot be NULL)
 *           pubkey:    pointer to an initialized public key to verify with (cannot be NULL)
 *
 * To avoid accepting malleable signatures, only ECDSA signatures in lower-S
 * form are accepted.
 *
 * If you need to accept ECDSA signatures from sources that do not obey this
 * rule, apply secp256k1_ecdsa_signature_normalize to the signature prior to
 * validation, but be aware that doing so results in malleable signatures.
 *
 * For details, see the comments for that function.
 */
int secp256k1_ecdsa_verify_ex(
    const secp256k1_context* ctx,
    const secp256k1_ecdsa_signature *sig,
    const unsigned char *msg32,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Tweak a public key by adding tweak times the generator to it.
 * Returns: 0 if the tweak was out of range (chance of around 1 in 2^128 for
 *          uniformly random 32-byte arrays, or if the resulting public key
 *          would be invalid (only when the tweak is the complement of the
 *          corresponding private key). 1 otherwise.
 * Args:    ctx:    pointer to a context object initialized for validation
 *                  (cannot be NULL).
 * In/Out:  pubkey: pointer to a public key object.
 * In:      tweak:  pointer to a 32-byte tweak.
 */
int secp256k1_ec_pubkey_tweak_add_ex(
    const secp256k1_context* ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

# ifdef __cplusplus
}
# endif

#endif
