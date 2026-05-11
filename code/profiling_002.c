#include <stdio.h>
#include <stdlib.h>

static void merge_ranges(double *values, double *buffer, int left, int mid, int right)
{
    int i = left;
    int j = mid;
    int k = left;

    while (i < mid && j < right)
    {
        if (values[i] <= values[j])
        {
            buffer[k++] = values[i++];
        }
        else
        {
            buffer[k++] = values[j++];
        }
    }

    while (i < mid)
    {
        buffer[k++] = values[i++];
    }

    while (j < right)
    {
        buffer[k++] = values[j++];
    }

    for (int index = left; index < right; index++)
    {
        values[index] = buffer[index];
    }
}

static void mergesort_recursive(double *values, double *buffer, int left, int right)
{
    if ((right - left) < 2)
    {
        return;
    }

    int mid = left + (right - left) / 2;
    mergesort_recursive(values, buffer, left, mid);
    mergesort_recursive(values, buffer, mid, right);
    merge_ranges(values, buffer, left, mid, right);
}

static int mergesort(double *values, int n)
{
    double *buffer = malloc((size_t)n * sizeof(double));
    if (buffer == NULL)
    {
        fprintf(stderr, "Error: could not allocate merge buffer\n");
        return 0;
    }

    mergesort_recursive(values, buffer, 0, n);
    free(buffer);
    return 1;
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

    printf("Id de programa       : 002\n");
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

    if (!mergesort(values, n))
    {
        free(values);
        return 1;
    }

    print_report(values, n);

    free(values);
    return 0;
}
