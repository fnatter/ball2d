#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <math.h>

#define datalen         5000
struct dataItem
{
      float data;
      int index;
};

dataItem data[datalen];

void quicksort_data(int first, int last)
{
int pivot_index, down, up;
dataItem temp;
float pivot;
pivot_index = (first + last) / 2;
pivot = data[pivot_index].data;
down = first;
up = last;
while (down < up)
      {
      while (data[down].data <= pivot && down <= last)
            down++;
      while (data[up].data > pivot && up >= first)
            up--;
      if (up > down)
            {
            temp = data[up];
            data[up] = data[down];
            data[down] = temp;
            }
      }
temp = data[pivot_index];
data[pivot_index] = data[up];
data[up] = temp;
pivot_index = up;
if (pivot_index > first + 1)
      quicksort_data(first, pivot_index - 1);
if (pivot_index < last - 1)
      quicksort_data(pivot_index + 1, last);
};

void vecswap(int a[], int i, int j, int n)
{
int temp;
while (n-- > 0)
      {
      temp = a[i];
      a[i] = a[j];
      a[j] = temp;
      i++;
      j++;
      }
};

void quicksort3(int a[], int n)
{
int le, lt, gt, ge, r, temp;
int v;
if (n <= 1)
      return;
le = lt = 1;
gt = ge = n - 1;
v = a[0];
for (; ;)
      {
      for (; lt <= gt && a[lt] <= v; lt++)
            if (a[lt] == v)
                  {
                  temp = a[lt];
                  a[lt] = a[le];
                  a[le] = temp;
                  le++;
                  }
      for (; lt <= gt && a[gt] >= v; gt--)
            {
            if (a[gt] == v)
                  {
                  temp = a[gt];
                  a[gt] = a[ge];
                  a[ge] = temp;
                  ge--;
                  }
            }
      if (lt > gt)
            break;
      temp = a[lt];
      a[lt] = a[gt];
      a[gt] = temp;
      lt++;
      gt--;
      }
r = le <? (lt - le);
vecswap(a, 0, lt - r, r);
r = (ge - gt) <? (n - ge - 1);
vecswap(a, lt, n - r, r);
quicksort3(a, lt - le);
quicksort3(a + n - (ge - gt), ge - gt);
};


void vecswap2(int a[], int i, int j, int n)
{
int temp;
while (n-- > 0)
      {
      temp = a[i];
      a[i] = a[j];
      a[j] = temp;
      i++;
      j++;
      }
};

void quicksort3_2(int a[], int n)
{
int le, lt, gt, ge, r, temp;
int v;
if (n <= 1)
      return;
le = lt = 1;
gt = ge = n - 1;
v = a[0];
for (; ;)
      {
      for (; lt <= gt && a[lt] <= v; lt++)
            if (a[lt] == v)
                  {
                  temp = a[lt];
                  a[lt] = a[le];
                  a[le] = temp;
                  le++;
                  }
      for (; lt <= gt && a[gt] >= v; gt--)
            {
            if (a[gt] == v)
                  {
                  temp = a[gt];
                  a[gt] = a[ge];
                  a[ge] = temp;
                  ge--;
                  }
            }
      if (lt > gt)
            break;
      temp = a[lt];
      a[lt] = a[gt];
      a[gt] = temp;
      lt++;
      gt--;
      }
r = le <? (lt - le);
vecswap2(a, 0, lt - r, r);
r = (ge - gt) <? (n - ge - 1);
vecswap2(a, lt, n - r, r);
quicksort3_2(a, lt - le);
quicksort3_2(a + n - (ge - gt), ge - gt);
};

void xchgsort_data()
{
int i, j;
dataItem temp;
for (i = 0; i < datalen - 1; i++)
      {
      for (j = i + 1; j < datalen; j++)
            {
            if (data[i].data > data[j].data)
                  {
                  temp = data[i];
                  data[i] = data[j];
                  data[j] = temp;
                  }
            }
      }
};

void bubblesort_data()
{//wer ist die sch”nste im ganzen land ? (die romantische version)
int Bound, j, t;
dataItem temp;
Bound = datalen - 1;
begin:
t = 0;
for (j = 0; j < Bound; j++)
	{
	if (data[j].data > data[j + 1].data)
		{
            temp = data[j];
            data[j] = data[j + 1];
            data[j + 1] = temp;
            t = j;
		}
	}
if (t != 0)
      {
      Bound = t;
      goto begin;
      }
};

void simple_bubblesort_data()
{
int i, t, Bound;
dataItem temp;
Bound = datalen;
begin:
t = 0;
for (i = datalen - 2; i >= 0; i--)
      {
      if (data[i].data > data[i + 1].data)
            {
            temp = data[i];
            data[i] = data[i + 1];
            data[i + 1] = temp;
            t = 1;
            }
      }
if (t != 0)
     goto begin;
};

void scramble()
{
int i;
for (i = 0; i < datalen; i++)
      {
      data[i].data = (float)(datalen - i);
      data[i].index = i;
      }
};

void report_errors()
{
int i, j = 0;
for (i = 1; i < datalen; i++)
      {
      if (data[i].data < data[i - 1].data)
            {
            printf("Error at data element %i.\n", i);
            j++;
            }
      }
if (j != 0)
      getch();
};

void check_scramble()
{
int i, j = 0;
for (i = 1; i < datalen; i++)
      {
      if (data[i].data >= data[i - 1].data)
            j++;
      }
printf("%d element transitions sorted! after scramble!\n", j);
};

int get_secs_100()
{
time tim;
gettime(&tim);
return (tim.ti_min * 6000) + (tim.ti_hour * 360000) + (tim.ti_sec * 100) + tim.ti_hund;
};

int main()
{
int i, timenow, timebef;
printf("\nSorting test...\n");
//quicksort3(data, datalen);
scramble();
check_scramble();
printf("Sorting set of %i with quicksort now...\n", datalen);
timebef = get_secs_100();
quicksort_data(0, datalen - 1);
timenow = get_secs_100();
printf("took about %f secs\n", (float)(timenow - timebef) / 100.0F);
report_errors();
scramble();
check_scramble();
printf("Sorting set of %i with xchgsort now...\n", datalen);
timebef = get_secs_100();
xchgsort_data();
timenow = get_secs_100();
printf("took about %f secs\n", (float)(timenow - timebef) / 100.0F);
report_errors();
scramble();
check_scramble();
printf("Sorting set of %i with bubblesort now...\n", datalen);
timebef = get_secs_100();
bubblesort_data();
timenow = get_secs_100();
printf("took about %f secs\n", (float)(timenow - timebef) / 100.0F);
report_errors();
scramble();
check_scramble();
printf("Sorting set of %i with simple bubblesort now...\n", datalen);
timebef = get_secs_100();
simple_bubblesort_data();
timenow = get_secs_100();
printf("took about %f secs\n", (float)(timenow - timebef) / 100.0F);
report_errors();
return 0;
};

/*
void quicksort_data_old(int first, int last)
{// wer ist die sch”nste im ganzen land ? (die schnelle version)
int up, down, pivot_index;
float pivot, temp;
up = first;
down = last;
pivot_index = first;
pivot = data[pivot_index];
while (down < up)
      {
      while (data[down] >= pivot && down <= last)
            down++;
      while (data[up] < pivot && up >= first)
            up--;
      if (up > down)
            {
            temp = data[up];
            data[up] = data[down];
            data[down] = temp;
            }
      }
temp = data[up];
data[up] = data[pivot_index];
data[pivot_index] = temp;
pivot_index = up;
if (pivot_index > first + 1)
      quicksort_data_old(first, pivot_index - 1);
if (pivot_index < last - 1)
      quicksort_data_old(pivot_index + 1, last);
};
*/

