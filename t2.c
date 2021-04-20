/**
 * MC558 - Teste 02
 * Tiago de Paula - RA 187679
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifdef __GNUC__
// Atributos do GCC
#define attribute(...) __attribute__((__VA_ARGS__))
// Marcador de branch provável e improvável
#define likely(x)      (__builtin_expect((x), 1))
#define unlikely(x)    (__builtin_expect((x), 0))

#else
// Fora do GCC, as macros não fazem nada.
#define attribute(...)
#define unlikely(x)    ((x))
#define likely(x)      ((x))
#endif

// Maior tamamnho de um vetor
#define MAX 1000

// Elemento do vetor: a_i <= 1000 < 2^16
typedef uint16_t item_t;
// Um vetor, com tamanho acoplado.
typedef struct vec {
    size_t len;
    item_t v[MAX];
} vec_t;

/* * * * * * * * * *
 * ENTRADA E SAÍDA *
 * * * * * * * * * */

static inline attribute(cold, nothrow)
/**
 * Leitura da tupla de entrada e do valor de N.
 *
 * Encerra execução em erro de leitura.
 */
vec_t read_input_tuple(size_t *N) {
    size_t n;
    int rv = scanf("%zu", &n);
    // N deve ser um inteiro e deve ser entre 2 e MAX
    if unlikely(rv < 1 || n < 2 || n > MAX) exit(EXIT_FAILURE);

    vec_t tuple = { .len = n - 2 };
    for (size_t i = 0; i < tuple.len; i++) {
        uint64_t ai;
        int rv = scanf("%"SCNu64, &ai);
        // elementos da tupla devem ser inteiros entre 1 e n
        if unlikely(rv < 1 || ai < 1 || ai > n) exit(EXIT_FAILURE);
        // se for válido, pode ser convertido para 16 bits
        tuple.v[i] = (item_t) (ai - 1);
    }
    *N = n;
    return tuple;
}

static inline attribute(const, cold, nothrow)
/**
 * Quantidade de dígitos decimais para representar um número.
 */
size_t digits(size_t num) {
    if (num == 0) return 1;

    size_t d = 0;
    for (; num > 0; num /= 10) {
        d += 1;
    }
    return d;
}

static inline attribute(hot, nothrow)
/**
 * Escreve dígitos no buffer e retorna ponteiro
 * para a próxima posição livre.
 */
char *write_unsigned(char *buffer, unsigned num) {
    if unlikely(num == 0) {
        *buffer = '0';
        return buffer + 1;
    }

    char buf[digits(MAX - 1) + 1];
    size_t len = 0;
    for (; num > 0; num /= 10) {
        char digit = num % 10;
        buf[len++] = digit + '0';
    }

    for (size_t i = 0; i < len; i++) {
        buffer[i] = buf[len - i - 1];
    }
    return buffer + len;
}

static inline attribute(nonnull, cold, nothrow)
/**
 * Exibe o vetor na saída padrão.
 */
void print_array(const vec_t *array) {
    // buffer precisa de espaço para representar cada um dos
    // digitos de cada número, com o espaço ou quebra de linha,
    // além do caracter nulo ao final
    char buffer[MAX * (digits(MAX - 1) + 1) + 1];

    // constrói buffer de impressão
    char *buf = buffer;
    for (size_t i = 0; i < array->len; i++) {
        unsigned vi = (unsigned) array->v[i];

        if likely(i > 0) {
            // sem espaço no primeiro elemento
            *(buf++) = ' ';
        }
        buf = write_unsigned(buf, vi);
    }
    *(buf++) = '\n';
    *buf = '\0';

    // impressão do buffer com todos os elementos já escritos
    fputs(buffer, stdout);
}

static inline
/* Inversa de Prüfer. */
vec_t inverse_prufer(size_t n, const vec_t tuple)
attribute(const, hot, nothrow);

/* Entrada do programa. */
int main(void) {
    size_t n;
    // leitura da tupla
    vec_t tuple = read_input_tuple(&n);
    // cálculo dos predecessores
    vec_t pred = inverse_prufer(n, tuple);
    // impressão do resultado
    print_array(&pred);

    return EXIT_SUCCESS;
}

/* * * * * * * * * * * * *
 * RESOLUÇÃO DO PROBLEMA *
 * * * * * * * * * * * * */

static attribute(const, cold, nothrow)
/**
 * Vetor com as repetições de cada elemento.
 *
 * Os elementos do vetor devem ser menores que
 * `len` e `len` deve ser menor que `MAX`.
 */
vec_t counter(size_t len, const vec_t array) {
    vec_t cnt = { .len = len, .v = { 0 } };

    for (size_t i = 0; i < array.len; i++) {
        item_t ai = array.v[i];
        cnt.v[ai] += 1;
    }
    return cnt;
}

static attribute(nonnull, hot, nothrow)
/**
 * Função auxiliar do heap de mínimo.
 */
void min_heapify(vec_t *heap, size_t i) {
    item_t value = heap->v[i];
    do {
        // filhos esquerdo e direito
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;

        // posição do menor elemento
        size_t smallest = i;
        // valor do menor
        item_t cur = value;

        if (left < heap->len && heap->v[left] < cur) {
            smallest = left;
            cur = heap->v[left];
        }
        if (right < heap->len && heap->v[right] < cur) {
            smallest = right;
            cur = heap->v[right];
        }
        // insere o menor no atual
        heap->v[i] = cur;

        if (cur < value) {
            // continua a execução
            i = smallest;
        } else {
            // encerra
            i = heap->len;
        }
    } while (i < heap->len);
}

static attribute(nonnull, hot, nothrow)
/**
 * Recupera o menor elemento do heap.
 *
 * Encerra se o heap estiver vazio.
 */
item_t heap_pop(vec_t *heap) {
    if unlikely(heap->len == 0) exit(EXIT_FAILURE);

    item_t min = heap->v[0];
    heap->v[0] = heap->v[--heap->len];
    min_heapify(heap, 0);

    return min;
}

static attribute(nonnull, hot, nothrow)
/**
 * Insere elemento no vetor, mantendo a propriedade
 * do heap.
 *
 * Encerra se o heap estiver cheio.
 */
void heap_push(vec_t *heap, item_t value) {
    if unlikely(heap->len == MAX) exit(EXIT_FAILURE);
    size_t i = heap->len++;

    size_t parent = (i - 1) / 2;
    // troca com o pai, enquanto for menor
    while (i > 0 && heap->v[parent] > value) {
        heap->v[i] = heap->v[parent];
        i = parent;
        parent = (i - 1) / 2;
    }
    heap->v[i] = value;
}

static attribute(const, cold, nothrow)
/**
 * Constrói heap de mínimo com os elementos de
 * S = [0,n) que não aparecem na tupla.
 *
 * Os elementos da tupla devem ser menores que `n`.
 */
vec_t build_min_heap(size_t n, const vec_t tuple) {
    // marcador de quais elementos estão na tupla
    bool present[MAX] = { false };
    for (size_t i = 0; i < tuple.len; i++) {
        item_t ai = tuple.v[i];
        present[ai] = true;
    }

    // constroi vetor apenas com elementos que
    // não estão presentes na tupla
    vec_t heap = { .len = 0 };
    for (size_t i = 0; i < n; i++) {
        if (! present[i]) {
            heap.v[heap.len++] = (item_t) i;
        }
    }
    // transforma em heap de mínimo
    for (size_t i = n / 2; i > 0; i--) {
        min_heapify(&heap, i-1);
    }
    return heap;
}

/**
 * Calcula a inversa do código de Prüfer, retornando
 * a árvore em um vetor dos predecessores.
 *
 * Os predecessores já são incrementados em 1.
 */
vec_t inverse_prufer(size_t n, const vec_t tuple) {
    // vetor dos predecessores
    vec_t pred = { .len = n };
    // contador de repetições na tupla
    vec_t cnt = counter(n, tuple);
    // fila das folhas, no caso, todos os vértices que não
    // aparecessem na tupla
    vec_t heap = build_min_heap(n, tuple);

    for (size_t i = 0; i < tuple.len; i++) {
        // menor folha da subarvore atual
        item_t v = heap_pop(&heap);
        // predecessor dele
        item_t ai = tuple.v[i];
        // cria aresta entre eles
        pred.v[v] = ai + 1;

        // se for a última aparição de `ai` na tupla,
        // então ele será considerado uma folha
        if (--cnt.v[ai] == 0) {
            heap_push(&heap, ai);
        }
    }
    // o menor dos vértices restantes é a raiz
    item_t x = heap_pop(&heap);
    pred.v[x] = 0;
    // que é predecessor do outro vértice
    item_t y = heap_pop(&heap);
    pred.v[y] = x + 1;

    return pred;
}
