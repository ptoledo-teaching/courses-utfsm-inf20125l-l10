#include <stdio.h>
#include <stdlib.h>

static void swap_double(double *a, double *b)
{
    double tmp = *a;
    *a = *b;
    *b = tmp;
}

static int partition_range(double *values, int low, int high)
{
    int mid = low + (high - low) / 2;
    double pivot = values[mid];

    swap_double(&values[mid], &values[high]);

    int store = low;
    for (int i = low; i < high; i++)
    {
        if (values[i] < pivot)
        {
            swap_double(&values[i], &values[store]);
            store++;
        }
    }

    swap_double(&values[store], &values[high]);
    return store;
}

static void quicksort_recursive(double *values, int low, int high)
{
    if (low >= high)
    {
        return;
    }

    int pivot_index = partition_range(values, low, high);
    quicksort_recursive(values, low, pivot_index - 1);
    quicksort_recursive(values, pivot_index + 1, high);
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

    printf("Id de programa       : 003\n");
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

    quicksort_recursive(values, 0, n - 1);
    print_report(values, n);

    free(values);
    return 0;
}
