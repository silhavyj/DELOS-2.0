int main() {
    *(char *)0xFFFFFFFF = 'A';
    return 0;
}