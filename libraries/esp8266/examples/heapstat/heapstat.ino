
#define N 10
#define S 384

void* p[N];
String* q[N];

extern "C" void    malloc_stat_print_all();

void setup ()
{
    Serial.begin(74880);
    
    Serial.printf("#######################################################\n");

    Serial.printf("FREE1\n");
    malloc_stat_print_all();
    
    for (int i = 0; i < N; i++)
    {
        yield();
        Serial.printf("MALLOC %d\n",i);
        p[i] = malloc(S);
    }
    
    Serial.printf("FREE1\n");
    malloc_stat_print_all();

    for (int i = 0; i < N; i++)
    {
        yield();
        q[i] = new String();
    }
    
    Serial.printf("FREE1\n");
    malloc_stat_print_all();
    
    for (int i = 0; i < N - 1; i++)
    {
        yield();
        free(p[i]);
    }

    Serial.printf("FREE1\n");
    malloc_stat_print_all();
    
    free(p[N-1]);

    for (int i = 0; i < N; i++)
    {
        yield();
        Serial.printf("FREE %d\n",i);
        delete q[i];
    }
    Serial.printf("DELETE2\n");
    malloc_stat_print_all();
}


String blah = "x";

void loop ()
{
#if 1
    blah.concat("#######################################################");
    //blah += "#######################################################";
    //Serial.printf("%u\n", blah.length());
#else
    for (int i = 0; i < 10000; i++)
        delay(1);
    malloc_stat_print_all();
#endif
}
