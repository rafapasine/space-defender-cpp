// =============================================================================
//  Space Defender C++
//  Jogo 2D de tiro espacial feito com Raylib, num único arquivo.
//
//  Controles:
//    - Setas Esquerda/Direita ou A/D : mover a nave
//    - Espaço                        : atirar
//    - ENTER                         : começar (tela de título)
//    - R                             : reiniciar (tela de Game Over)
//
//  Todos os gráficos são desenhados com primitivas da Raylib
//  (DrawTriangle, DrawRectangle, DrawCircle, DrawPixel); não há assets externos.
// =============================================================================

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// Constantes de configuração
// -----------------------------------------------------------------------------
namespace Config {
    constexpr int   SCREEN_W        = 800;
    constexpr int   SCREEN_H        = 600;
    constexpr int   TARGET_FPS      = 60;
    constexpr char  TITLE[]         = "Space Defender C++";

    constexpr int   STAR_COUNT      = 120;

    constexpr float PLAYER_SPEED    = 380.0f;   // pixels por segundo
    constexpr float PLAYER_HALF_W   = 20.0f;
    constexpr float PLAYER_HEIGHT   = 32.0f;
    constexpr float SHOT_COOLDOWN   = 0.22f;    // segundos entre tiros
    constexpr float INVULN_TIME     = 1.5f;     // invencibilidade após levar dano
    constexpr int   START_LIVES     = 3;

    constexpr float BULLET_SPEED    = 620.0f;
    constexpr float BULLET_W        = 4.0f;
    constexpr float BULLET_H        = 14.0f;

    constexpr float ENEMY_RADIUS    = 18.0f;    // raio usado na colisão
    constexpr float ENEMY_BASE_SPD  = 110.0f;   // velocidade inicial
    constexpr float ENEMY_MAX_SPD   = 420.0f;
    constexpr float SPAWN_BASE      = 1.20f;    // intervalo inicial entre inimigos (s)
    constexpr float SPAWN_MIN       = 0.28f;    // intervalo mínimo

    constexpr int   POINTS_PER_KILL = 10;
}

// -----------------------------------------------------------------------------
// Estados do jogo
// -----------------------------------------------------------------------------
enum class GameState { Title, Playing, GameOver };

// -----------------------------------------------------------------------------
// Entidades
// -----------------------------------------------------------------------------
struct Star {
    Vector2 pos;
    float   speed;       // estrelas mais rápidas parecem mais próximas (parallax)
    int     size;        // 1 = pixel, 2+ = círculo pequeno
    Color   color;
};

struct Bullet {
    Vector2 pos;
    bool    active = true;
};

struct Enemy {
    Vector2 pos;
    float   speed;
    float   wobblePhase;   // fase do balanço lateral
    float   wobbleAmp;     // amplitude do balanço lateral
    float   baseX;         // posição X de referência para o balanço
    Color   color;
    bool    active = true;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float   life;
    float   maxLife;
    float   size;
    Color   color;
};

struct Shockwave {          // círculo que se expande na explosão
    Vector2 pos;
    float   radius;
    float   life;
    float   maxLife;
};

struct Player {
    Vector2 pos;            // centro da base da nave
    float   shotTimer   = 0.0f;
    float   invulnTimer = 0.0f;
    int     lives       = Config::START_LIVES;
};

// -----------------------------------------------------------------------------
// Funções utilitárias
// -----------------------------------------------------------------------------

// Número aleatório float no intervalo [min, max]
static float RandF(float min, float max) {
    return min + (max - min) * (GetRandomValue(0, 10000) / 10000.0f);
}

// Desenha um triângulo garantindo a ordem anti-horária exigida pela Raylib.
// (Se a ordem estiver errada, a Raylib simplesmente não desenha o triângulo.)
static void DrawTri(Vector2 a, Vector2 b, Vector2 c, Color color) {
    const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    // Com o eixo Y apontando para baixo (tela), ordem anti-horária => cross < 0
    if (cross < 0.0f) DrawTriangle(a, b, c, color);   // já está na ordem correta
    else              DrawTriangle(a, c, b, color);   // inverte b e c
}

// Distância ao quadrado entre dois pontos (evita sqrt nas colisões)
static float Dist2(Vector2 a, Vector2 b) {
    const float dx = a.x - b.x, dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// -----------------------------------------------------------------------------
// Classe principal do jogo
// -----------------------------------------------------------------------------
class Game {
public:
    Game() {
        InitStars();
        Reset();
    }

    // Um quadro: atualização + desenho
    void Frame() {
        const float dt = GetFrameTime();
        Update(dt);
        Draw();
    }

private:
    GameState              state_      = GameState::Title;
    Player                 player_;
    std::vector<Star>      stars_;
    std::vector<Bullet>    bullets_;
    std::vector<Enemy>     enemies_;
    std::vector<Particle>  particles_;
    std::vector<Shockwave> shockwaves_;
    int                    score_      = 0;
    int                    highScore_  = 0;
    float                  spawnTimer_ = 0.0f;
    float                  timeAlive_  = 0.0f;  // usado em animações
    float                  screenShake_ = 0.0f;

    // ---------------------------------------------------------------------
    // Inicialização
    // ---------------------------------------------------------------------
    void InitStars() {
        stars_.clear();
        stars_.reserve(Config::STAR_COUNT);
        for (int i = 0; i < Config::STAR_COUNT; ++i) {
            stars_.push_back(MakeStar(RandF(0, Config::SCREEN_H)));
        }
    }

    // Cria uma estrela em uma altura y específica; a camada define brilho/velocidade
    static Star MakeStar(float y) {
        const int layer = GetRandomValue(0, 2);           // 0 = longe, 2 = perto
        Star s;
        s.pos   = { RandF(0, Config::SCREEN_W), y };
        s.speed = 30.0f + layer * 60.0f + RandF(0, 20);
        s.size  = (layer == 2) ? 2 : 1;
        const unsigned char b = static_cast<unsigned char>(110 + layer * 60);
        s.color = { b, b, static_cast<unsigned char>(std::min(255, b + 30)), 255 };
        return s;
    }

    // Reinicia a partida (mantém o recorde)
    void Reset() {
        player_       = Player{};
        player_.pos   = { Config::SCREEN_W / 2.0f, Config::SCREEN_H - 30.0f };
        bullets_.clear();
        enemies_.clear();
        particles_.clear();
        shockwaves_.clear();
        score_        = 0;
        spawnTimer_   = 1.0f;   // pequena pausa antes do primeiro inimigo
        screenShake_  = 0.0f;
    }

    // ---------------------------------------------------------------------
    // Dificuldade: depende da pontuação
    // ---------------------------------------------------------------------
    float CurrentEnemySpeed() const {
        return std::min(Config::ENEMY_MAX_SPD,
                        Config::ENEMY_BASE_SPD + score_ * 0.9f);
    }

    float CurrentSpawnInterval() const {
        return std::max(Config::SPAWN_MIN,
                        Config::SPAWN_BASE - score_ * 0.0045f);
    }

    // ---------------------------------------------------------------------
    // Atualização
    // ---------------------------------------------------------------------
    void Update(float dt) {
        timeAlive_ += dt;
        UpdateStars(dt);           // o fundo se move em todas as telas
        UpdateEffects(dt);         // explosões continuam mesmo no Game Over

        switch (state_) {
            case GameState::Title:
                if (IsKeyPressed(KEY_ENTER)) {
                    Reset();
                    state_ = GameState::Playing;
                }
                break;

            case GameState::Playing:
                UpdatePlayer(dt);
                UpdateBullets(dt);
                SpawnEnemies(dt);
                UpdateEnemies(dt);
                HandleCollisions();
                RemoveInactive();
                if (player_.lives <= 0) {
                    highScore_ = std::max(highScore_, score_);
                    state_ = GameState::GameOver;
                }
                break;

            case GameState::GameOver:
                if (IsKeyPressed(KEY_R)) {
                    Reset();
                    state_ = GameState::Playing;
                }
                break;
        }
    }

    void UpdateStars(float dt) {
        // No jogo, o fundo acelera levemente junto com a dificuldade
        const float boost = (state_ == GameState::Playing)
                          ? 1.0f + score_ * 0.002f : 1.0f;
        for (auto& s : stars_) {
            s.pos.y += s.speed * boost * dt;
            if (s.pos.y > Config::SCREEN_H) {
                s = MakeStar(RandF(-20.0f, 0.0f));   // recicla no topo
            }
        }
    }

    void UpdatePlayer(float dt) {
        float dir = 0.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dir -= 1.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir += 1.0f;

        player_.pos.x += dir * Config::PLAYER_SPEED * dt;
        player_.pos.x  = std::clamp(player_.pos.x,
                                    Config::PLAYER_HALF_W,
                                    Config::SCREEN_W - Config::PLAYER_HALF_W);

        player_.shotTimer   = std::max(0.0f, player_.shotTimer - dt);
        player_.invulnTimer = std::max(0.0f, player_.invulnTimer - dt);

        // Segurar espaço dispara continuamente, respeitando o cooldown
        if (IsKeyDown(KEY_SPACE) && player_.shotTimer <= 0.0f) {
            bullets_.push_back({ { player_.pos.x,
                                   player_.pos.y - Config::PLAYER_HEIGHT } });
            player_.shotTimer = Config::SHOT_COOLDOWN;
        }
    }

    void UpdateBullets(float dt) {
        for (auto& b : bullets_) {
            b.pos.y -= Config::BULLET_SPEED * dt;
            if (b.pos.y + Config::BULLET_H < 0) b.active = false;
        }
    }

    void SpawnEnemies(float dt) {
        spawnTimer_ -= dt;
        if (spawnTimer_ > 0.0f) return;

        // Intervalo com uma pequena variação aleatória
        spawnTimer_ = CurrentSpawnInterval() * RandF(0.7f, 1.3f);

        static const Color palette[] = {
            { 230,  60,  80, 255 },   // vermelho
            { 180,  70, 220, 255 },   // roxo
            { 240, 140,  40, 255 },   // laranja
            {  60, 200, 120, 255 },   // verde
        };

        Enemy e;
        const float margin = Config::ENEMY_RADIUS + 10.0f;
        e.baseX       = RandF(margin, Config::SCREEN_W - margin);
        e.pos         = { e.baseX, -Config::ENEMY_RADIUS * 2 };
        e.speed       = CurrentEnemySpeed() * RandF(0.8f, 1.2f);
        e.wobblePhase = RandF(0.0f, 2.0f * PI);
        e.wobbleAmp   = RandF(0.0f, 40.0f);
        e.color       = palette[GetRandomValue(0, 3)];
        enemies_.push_back(e);
    }

    void UpdateEnemies(float dt) {
        for (auto& e : enemies_) {
            e.pos.y       += e.speed * dt;
            e.wobblePhase += 2.5f * dt;
            e.pos.x = std::clamp(e.baseX + std::sin(e.wobblePhase) * e.wobbleAmp,
                                 Config::ENEMY_RADIUS,
                                 Config::SCREEN_W - Config::ENEMY_RADIUS);

            // Inimigo passou pela defesa: o jogador perde uma vida
            if (e.pos.y - Config::ENEMY_RADIUS > Config::SCREEN_H) {
                e.active = false;
                LoseLife();
            }
        }
    }

    void HandleCollisions() {
        const float hitR2 = (Config::ENEMY_RADIUS + 4.0f) * (Config::ENEMY_RADIUS + 4.0f);

        // Tiro x inimigo
        for (auto& b : bullets_) {
            if (!b.active) continue;
            for (auto& e : enemies_) {
                if (!e.active) continue;
                const Vector2 bulletCenter = { b.pos.x, b.pos.y + Config::BULLET_H / 2 };
                if (Dist2(bulletCenter, e.pos) < hitR2) {
                    b.active = false;
                    e.active = false;
                    score_  += Config::POINTS_PER_KILL;
                    SpawnExplosion(e.pos, e.color);
                    break;
                }
            }
        }

        // Inimigo x jogador (usa um círculo aproximado para a nave)
        if (player_.invulnTimer > 0.0f) return;
        const Vector2 shipCenter = { player_.pos.x,
                                     player_.pos.y - Config::PLAYER_HEIGHT / 2 };
        const float shipR = 16.0f;
        const float r2 = (shipR + Config::ENEMY_RADIUS) * (shipR + Config::ENEMY_RADIUS);
        for (auto& e : enemies_) {
            if (e.active && Dist2(shipCenter, e.pos) < r2) {
                e.active = false;
                SpawnExplosion(e.pos, e.color);
                SpawnExplosion(shipCenter, SKYBLUE);
                LoseLife();
                break;
            }
        }
    }

    void LoseLife() {
        if (player_.lives <= 0) return;
        --player_.lives;
        player_.invulnTimer = Config::INVULN_TIME;
        screenShake_ = 0.35f;
    }

    // Remove tiros e inimigos inativos (idioma erase-remove)
    void RemoveInactive() {
        bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(),
                       [](const Bullet& b) { return !b.active; }), bullets_.end());
        enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(),
                       [](const Enemy& e) { return !e.active; }), enemies_.end());
    }

    // ---------------------------------------------------------------------
    // Efeitos de explosão
    // ---------------------------------------------------------------------
    void SpawnExplosion(Vector2 pos, Color baseColor) {
        // Partículas espalhadas em todas as direções
        const int count = GetRandomValue(18, 28);
        for (int i = 0; i < count; ++i) {
            const float angle = RandF(0.0f, 2.0f * PI);
            const float speed = RandF(60.0f, 260.0f);
            Particle p;
            p.pos     = pos;
            p.vel     = { std::cos(angle) * speed, std::sin(angle) * speed };
            p.maxLife = p.life = RandF(0.35f, 0.8f);
            p.size    = RandF(1.5f, 3.5f);
            // Alterna entre a cor do inimigo, amarelo e branco
            const int pick = GetRandomValue(0, 2);
            p.color = (pick == 0) ? baseColor : (pick == 1 ? YELLOW : WHITE);
            particles_.push_back(p);
        }
        // Onda de choque circular
        shockwaves_.push_back({ pos, 4.0f, 0.35f, 0.35f });
    }

    void UpdateEffects(float dt) {
        for (auto& p : particles_) {
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;
            p.vel.x *= 0.96f;              // atrito leve
            p.vel.y *= 0.96f;
            p.life  -= dt;
        }
        particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                         [](const Particle& p) { return p.life <= 0.0f; }),
                         particles_.end());

        for (auto& s : shockwaves_) {
            s.radius += 140.0f * dt;
            s.life   -= dt;
        }
        shockwaves_.erase(std::remove_if(shockwaves_.begin(), shockwaves_.end(),
                          [](const Shockwave& s) { return s.life <= 0.0f; }),
                          shockwaves_.end());

        screenShake_ = std::max(0.0f, screenShake_ - dt);
    }

    // ---------------------------------------------------------------------
    // Desenho
    // ---------------------------------------------------------------------
    void Draw() const {
        BeginDrawing();
        ClearBackground({ 5, 5, 20, 255 });

        // Tremor de tela quando o jogador é atingido
        Vector2 shake = { 0, 0 };
        if (screenShake_ > 0.0f) {
            const float m = screenShake_ * 18.0f;
            shake = { RandF(-m, m), RandF(-m, m) };
        }

        DrawStars();

        switch (state_) {
            case GameState::Title:
                DrawTitleScreen();
                break;

            case GameState::Playing:
                DrawWorld(shake);
                DrawHUD();
                break;

            case GameState::GameOver:
                DrawWorld(shake);
                DrawHUD();
                DrawGameOverScreen();
                break;
        }

        EndDrawing();
    }

    void DrawStars() const {
        for (const auto& s : stars_) {
            if (s.size <= 1) DrawPixel(static_cast<int>(s.pos.x), static_cast<int>(s.pos.y), s.color);
            else             DrawCircle(static_cast<int>(s.pos.x), static_cast<int>(s.pos.y), s.size * 0.6f, s.color);
        }
    }

    void DrawWorld(Vector2 shake) const {
        for (const auto& e : enemies_) DrawEnemy(e, shake);
        for (const auto& b : bullets_) DrawBullet(b, shake);

        // Pisca durante a invencibilidade; some no Game Over
        const bool blinkHidden = player_.invulnTimer > 0.0f &&
                                 static_cast<int>(player_.invulnTimer * 12) % 2 == 0;
        if (state_ == GameState::Playing && !blinkHidden) DrawShip(shake);

        DrawEffects(shake);
    }

    // Nave do jogador: corpo triangular, asas, cabine e chama do motor
    void DrawShip(Vector2 shake) const {
        const float x = player_.pos.x + shake.x;
        const float y = player_.pos.y + shake.y;
        const float h = Config::PLAYER_HEIGHT;
        const float w = Config::PLAYER_HALF_W;

        // Chama do motor (tremula com o tempo)
        const float flame = 8.0f + std::sin(timeAlive_ * 40.0f) * 3.0f;
        DrawTri({ x - 6, y }, { x + 6, y }, { x, y + flame + 4 }, ORANGE);
        DrawTri({ x - 3, y }, { x + 3, y }, { x, y + flame },     YELLOW);

        // Asas
        DrawTri({ x - w, y }, { x - 6, y - h * 0.55f }, { x - 4, y }, { 40, 110, 200, 255 });
        DrawTri({ x + w, y }, { x + 6, y - h * 0.55f }, { x + 4, y }, { 40, 110, 200, 255 });

        // Corpo principal
        DrawTri({ x, y - h }, { x - 9, y }, { x + 9, y }, { 90, 180, 255, 255 });

        // Canhões nas pontas das asas
        DrawRectangle(static_cast<int>(x - w),     static_cast<int>(y - 10), 3, 10, LIGHTGRAY);
        DrawRectangle(static_cast<int>(x + w - 3), static_cast<int>(y - 10), 3, 10, LIGHTGRAY);

        // Cabine
        DrawCircle(static_cast<int>(x), static_cast<int>(y - h * 0.45f), 4.0f, { 200, 240, 255, 255 });
    }

    // Inimigo: nave alienígena apontando para baixo
    void DrawEnemy(const Enemy& e, Vector2 shake) const {
        const float x = e.pos.x + shake.x;
        const float y = e.pos.y + shake.y;
        const float r = Config::ENEMY_RADIUS;
        const Color dark = { static_cast<unsigned char>(e.color.r / 2),
                             static_cast<unsigned char>(e.color.g / 2),
                             static_cast<unsigned char>(e.color.b / 2), 255 };

        // Asas laterais
        DrawTri({ x - r, y - r * 0.6f }, { x - r * 0.3f, y - r * 0.2f }, { x - r * 0.5f, y + r * 0.7f }, dark);
        DrawTri({ x + r, y - r * 0.6f }, { x + r * 0.3f, y - r * 0.2f }, { x + r * 0.5f, y + r * 0.7f }, dark);

        // Corpo em forma de seta apontando para baixo
        DrawTri({ x - r * 0.6f, y - r * 0.6f }, { x + r * 0.6f, y - r * 0.6f }, { x, y + r }, e.color);

        // Núcleo luminoso pulsante
        const float pulse = 3.5f + std::sin(timeAlive_ * 8.0f + e.wobblePhase) * 1.2f;
        DrawCircle(static_cast<int>(x), static_cast<int>(y - r * 0.15f), pulse + 1.5f, BLACK);
        DrawCircle(static_cast<int>(x), static_cast<int>(y - r * 0.15f), pulse, YELLOW);
    }

    // Laser: retângulo com núcleo claro
    void DrawBullet(const Bullet& b, Vector2 shake) const {
        const int x = static_cast<int>(b.pos.x - Config::BULLET_W / 2 + shake.x);
        const int y = static_cast<int>(b.pos.y + shake.y);
        DrawRectangle(x - 1, y, static_cast<int>(Config::BULLET_W) + 2,
                      static_cast<int>(Config::BULLET_H), { 0, 255, 120, 120 });
        DrawRectangle(x + 1, y + 2, static_cast<int>(Config::BULLET_W) - 2,
                      static_cast<int>(Config::BULLET_H) - 4, { 200, 255, 220, 255 });
    }

    void DrawEffects(Vector2 shake) const {
        // Ondas de choque: anel feito com dois círculos (externo colorido, interno "apagado")
        for (const auto& s : shockwaves_) {
            const float t = s.life / s.maxLife;   // 1 -> 0
            const auto alpha = static_cast<unsigned char>(200 * t);
            const int cx = static_cast<int>(s.pos.x + shake.x);
            const int cy = static_cast<int>(s.pos.y + shake.y);
            DrawCircle(cx, cy, s.radius, { 255, 200, 80, alpha });
            DrawCircle(cx, cy, std::max(0.0f, s.radius - 4.0f),
                       { 255, 255, 255, static_cast<unsigned char>(60 * t) });
        }
        // Partículas desaparecendo com o tempo
        for (const auto& p : particles_) {
            const float t = p.life / p.maxLife;
            Color c = p.color;
            c.a = static_cast<unsigned char>(255 * t);
            DrawCircle(static_cast<int>(p.pos.x + shake.x), static_cast<int>(p.pos.y + shake.y),
                       p.size * (0.5f + 0.5f * t), c);
        }
    }

    // HUD: pontuação e vidas no canto superior esquerdo
    void DrawHUD() const {
        DrawText(TextFormat("Pontos: %d", score_), 12, 10, 24, RAYWHITE);
        DrawText("Vidas:", 12, 40, 20, RAYWHITE);

        // Cada vida é uma mini-nave; vidas perdidas aparecem apagadas
        for (int i = 0; i < Config::START_LIVES; ++i) {
            const float x = 90.0f + i * 28.0f;
            const float y = 60.0f;
            const Color c = (i < player_.lives) ? Color{ 90, 180, 255, 255 }
                                                : Color{ 60, 60, 80, 255 };
            DrawTri({ x, y - 18 }, { x - 10, y }, { x + 10, y }, c);
        }

        // Nível de dificuldade aproximado, à direita
        const int level = 1 + score_ / 100;
        const char* lvl = TextFormat("Nivel %d", level);
        DrawText(lvl, Config::SCREEN_W - MeasureText(lvl, 20) - 12, 12, 20, GRAY);
    }

    // Texto centralizado horizontalmente
    static void DrawCentered(const char* text, int y, int size, Color color) {
        DrawText(text, (Config::SCREEN_W - MeasureText(text, size)) / 2, y, size, color);
    }

    void DrawTitleScreen() const {
        DrawCentered("SPACE DEFENDER", 170, 60, { 90, 180, 255, 255 });
        DrawCentered("C++ EDITION", 235, 24, LIGHTGRAY);

        // Texto piscando
        if (static_cast<int>(timeAlive_ * 2) % 2 == 0) {
            DrawCentered("Pressione ENTER para jogar", 330, 28, RAYWHITE);
        }

        DrawCentered("Setas / A-D: mover     Espaco: atirar", 420, 20, GRAY);
        if (highScore_ > 0) {
            DrawCentered(TextFormat("Recorde: %d", highScore_), 460, 20, GOLD);
        }
    }

    void DrawGameOverScreen() const {
        DrawRectangle(0, 0, Config::SCREEN_W, Config::SCREEN_H, { 0, 0, 0, 150 });
        DrawCentered("GAME OVER", 190, 64, { 230, 60, 80, 255 });
        DrawCentered(TextFormat("Pontuacao final: %d", score_), 280, 28, RAYWHITE);
        DrawCentered(TextFormat("Recorde: %d", highScore_), 320, 22, GOLD);
        if (static_cast<int>(timeAlive_ * 2) % 2 == 0) {
            DrawCentered("Game Over - Pressione R para reiniciar", 390, 26, RAYWHITE);
        }
    }
};

// -----------------------------------------------------------------------------
// Ponto de entrada
// -----------------------------------------------------------------------------
int main() {
    InitWindow(Config::SCREEN_W, Config::SCREEN_H, Config::TITLE);
    SetTargetFPS(Config::TARGET_FPS);
    SetExitKey(KEY_ESCAPE);   // ESC fecha o jogo

    {
        Game game;            // escopo garante destruição antes de CloseWindow
        while (!WindowShouldClose()) {
            game.Frame();
        }
    }

    CloseWindow();
    return 0;
}
