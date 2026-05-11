#include <stdio.h>
#include <stdlib.h>

static void swap_double(double *a, double *b)
{
    double tmp = *a;
    *a = *b;
    *b = tmp;
}

static void sift_down(double *values, int start, int end)
{
    int root = start;

    while ((2 * root + 1) <= end)
    {
        int child = 2 * root + 1;
        int candidate = root;

        if (values[candidate] < values[child])
        {
            candidate = child;
        }
        if ((child + 1) <= end && values[candidate] < values[child + 1])
        {
            candidate = child + 1;
        }
        if (candidate == root)
        {
            return;
        }

        swap_double(&values[root], &values[candidate]);
        root = candidate;
    }
}

static void heapsort(double *values, int n)
{
    if (n < 2)
    {
        return;
    }

    for (int start = (n - 2) / 2; start >= 0; start--)
    {
        sift_down(values, start, n - 1);
    }

    for (int end = n - 1; end > 0; end--)
    {
        swap_double(&values[0], &values[end]);
        sift_down(values, 0, end - 1);
    }
}

static int read_values(double **values_out, int *n_out)
{
    int n = 0;

    if (scanf("%d", &n) != 1 || n <= 0)
    {
        fprintf(stderr, "Error: invalid number of elements\n");
        return 0;
    }

    double *values = malloc((size_t)n * sizeof(double));
    if (values == NULL)
    {
        fprintf(stderr, "Error: could not allocate input array\n");
        return 0;
    }

    for (int i = 0; i < n; i++)
    {
        if (scanf("%lf", &values[i]) != 1)
        {
            fprintf(stderr, "Error: could not read value %d\n", i + 1);
            free(values);
            return 0;
        }
    }

    *values_out = values;
    *n_out = n;
    return 1;
}

static void print_report(const double *values, int n)
{
    double total = 0.0;
    double median = 0.0;

    for (int i = 0; i < n; i++)
    {
        total += values[i];
    }

    if ((n % 2) == 0)
    {
        median = (values[n / 2 - 1] + values[n / 2]) / 2.0;
    }
    else
    {
        median = values[n / 2];
    }

    printf("Id de programa       : 001\n");
    printf("Elementos procesados : %d\n", n);
    printf("Minimo               : %.2f\n", values[0]);
    printf("Mediana              : %.2f\n", median);
    printf("Maximo               : %.2f\n", values[n - 1]);
    printf("Promedio             : %.2f\n", total / (double)n);
    printf("Suma total           : %.2f\n", total);
}

int main(void)
{
    double *values = NULL;
    int n = 0;

    if (!read_values(&values, &n))
    {
        return 1;
    }

    heapsort(values, n);
    print_report(values, n);

    free(values);
    return 0;
}
