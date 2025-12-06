#include <chrono>
#include <numeric>

namespace bench
{

// Шаблонная функция для измерения времени выполнения
template <typename Number, typename Func, typename... Args>
long long benchmark(Func func, Args&&... args) {
    auto start = std::chrono::high_resolution_clock::now();
    // Выполняем функцию
    volatile Number result = func(std::forward<Args>(args)...); // volatile для предотвращения оптимизации компилятором
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    // Чтобы результат не был проигнорирован
    (void)result; 
    return duration;
}

void modulo_poly_calc();

}