int get_closest_lesser_n (int *arr, int len, int needle, int *index) {
  if (len < 2) {
    if(arr[0] < needle) return arr[0];
    else return -1;
  }
  int middle = len/2;
  if (arr[middle] < needle) {
    if (arr[middle + 1] >= needle) {
      *index += middle;
      return arr[middle];
    }
    else {
      *index += (middle+1);
      return get_closest_lesser_n(arr+(middle+1), len - (middle+1), needle, index);
    }
  }
  else return get_closest_lesser_n(arr, middle, needle, index);
}

int find (int *arr, int len, int needle) {
  for (int i = 0; i < len; i++) if(arr[i] == needle) return i;
  return -1;
}

