# Guia de Build e Uso — RAW DISK VIEWER CPLX

## Índice

1. [Requisitos](#1-requisitos)
2. [Compilar o Core C (Windows)](#2-compilar-o-core-c--windows)
3. [Compilar o Core C (Linux)](#3-compilar-o-core-c--linux)
4. [Compilar a Bridge JNI (Windows)](#4-compilar-a-bridge-jni--windows)
5. [Compilar a Bridge JNI (Linux)](#5-compilar-a-bridge-jni--linux)
6. [Build do GUI com Maven](#6-build-do-gui-com-maven)
7. [Executar o aplicativo](#7-executar-o-aplicativo)
8. [Demo Mode (sem biblioteca nativa)](#8-demo-mode)
9. [Estrutura de saída](#9-estrutura-de-saída)
10. [Fluxo de uso forense](#10-fluxo-de-uso-forense)
11. [Solução de problemas](#11-solução-de-problemas)

---

## 1. Requisitos

### Compilação C / JNI

| Item | Windows | Linux |
|---|---|---|
| Compilador | MinGW-w64 ≥ 12 (`gcc` no PATH) | GCC ≥ 12 (`build-essential`) |
| JDK | JDK 21+ com `JAVA_HOME` definido | JDK 21+ com `JAVA_HOME` definido |
| Make | Opcional | Opcional |

Verificar no terminal:

```bash
gcc --version        # deve retornar GCC 12+
java -version        # deve retornar 21
echo $JAVA_HOME      # Linux — deve apontar ao JDK
echo %JAVA_HOME%     # Windows — deve apontar ao JDK
```

### GUI / Java

| Item | Versão |
|---|---|
| JDK | 21+ |
| Apache Maven | 3.9+ |
| Internet | Necessária na 1ª execução (download deps) |

---

## 2. Compilar o Core C — Windows

Execute no **Prompt de Comando** ou **PowerShell** na raiz do projeto:

```bat
build\build-core-windows.bat
```

Saída esperada:
```
[RDVC] Building C Core (Windows x64)...
[OK] Output: build\output\rdvc_core.dll
```

O script compila todos os `.c` em `core/src/` com as flags:
- `-shared` — gera DLL
- `-DRDVC_BUILDING_DLL` — ativa macros de exportação `__declspec(dllexport)`
- `-DRDVC_WINDOWS` — seleciona implementação `CreateFile/ReadFile`
- `-lkernel32 -ladvapi32` — bibliotecas do sistema

---

## 3. Compilar o Core C — Linux

```bash
chmod +x build/build-core-linux.sh
./build/build-core-linux.sh
```

Saída esperada:
```
[RDVC] Building C Core (Linux x86-64)...
[OK] Output: build/output/librdvc_core.so
```

Flags usadas: `-shared -fPIC -DRDVC_LINUX -lm`

---

## 4. Compilar a Bridge JNI — Windows

Requer que o passo 2 já tenha sido executado:

```bat
build\build-jni-windows.bat
```

Saída esperada:
```
[RDVC] Building JNI Bridge (Windows x64)...
[OK] Output: build\output\rdvc_jni.dll

IMPORTANT: Copy rdvc_core.dll and rdvc_jni.dll next to your JAR...
```

O script procura automaticamente os headers JNI em `%JAVA_HOME%\include\win32`.

---

## 5. Compilar a Bridge JNI — Linux

```bash
./build/build-jni-linux.sh
```

Saída esperada:
```
[RDVC] Building JNI Bridge (Linux x86-64)...
[OK] Output: build/output/librdvc_jni.so
```

---

## 6. Build do GUI com Maven

```bash
cd gui
mvn clean package -q
```

Isso gera um fat JAR em `gui/target/raw-disk-viewer-cplx-1.0.0.jar` com todas as dependências (JavaFX + GSON) embutidas.

Para compilar e executar diretamente sem criar o JAR:

```bash
cd gui
mvn javafx:run
```

---

## 7. Executar o Aplicativo

### Opção A — Via Maven (desenvolvimento)

```bash
cd gui
mvn javafx:run
```

### Opção B — Via JAR (distribuição)

```bat
:: Windows — executar como Administrador para acessar discos físicos
java -Djava.library.path=build\output -jar gui\target\raw-disk-viewer-cplx-1.0.0.jar
```

```bash
# Linux
export LD_LIBRARY_PATH=build/output:$LD_LIBRARY_PATH
java -Djava.library.path=build/output -jar gui/target/raw-disk-viewer-cplx-1.0.0.jar
```

> **Importante (Windows):** Para acessar `\\.\PhysicalDrive0` e outros discos físicos,  
> o processo precisa de **privilégios de Administrador**.  
> Clique direito → "Executar como Administrador" no terminal ou no atalho do app.

---

## 8. Demo Mode

Se as bibliotecas nativas (`rdvc_core.dll` / `rdvc_jni.dll`) **não forem encontradas**,  
o aplicativo inicia automaticamente em **Demo Mode**:

- 2 discos falsos são listados (HDD e image file)
- Setores retornam dados sintéticos (padrão por LBA)
- O MBR no LBA 0 tem assinatura `0x55AA` e uma entrada de partição NTFS
- Todas as funcionalidades visuais estão acessíveis

O Demo Mode é indicado no log de auditoria: `DISK_OPEN  DEMO mode: ...`

---

## 9. Estrutura de Saída

Após o build completo:

```
build/output/
├── rdvc_core.dll       (Windows) ou librdvc_core.so (Linux)
└── rdvc_jni.dll        (Windows) ou librdvc_jni.so  (Linux)

gui/target/
└── raw-disk-viewer-cplx-1.0.0.jar   (fat JAR com JavaFX + GSON)
```

**Implantação mínima:**
```
minha-pasta/
├── raw-disk-viewer-cplx-1.0.0.jar
├── rdvc_core.dll
└── rdvc_jni.dll
```

---

## 10. Fluxo de Uso Forense

```
1. Abrir evidência
   ├── Discos físicos: Painel esquerdo → selecionar → duplo-clique
   └── Imagens:        File → Open Image File…  (ou botão "📂 Open Image File")

2. Navegar setores
   ├── Barra "SectorNavigator": digitar LBA ou usar ◀ / ▶
   ├── Roda do mouse: scroll no hex viewer
   ├── Navigate → Go To Sector…  (suporta hex com 0x)
   └── Navigate → Go To MBR / GPT Header

3. Analisar partições
   └── Painel direito → "Partition Table" (TreeView com MBR/GPT)

4. Varrer assinaturas
   └── Scan → Scan File Signatures…
       Resultado: lista de tipos encontrados por offset

5. Recuperar arquivos (carving)
   └── Scan → Carve Files…
       Resultado: lista de arquivos recuperados com tamanho estimado

6. Marcar posições importantes
   ├── Clique direito no hex viewer → adiciona bookmark
   └── Tools → Add Bookmark at Current LBA

7. Exportar evidência
   ├── Tools → Export Sectors…  (setor bruto → arquivo binário)
   └── Clique direito em arquivo recuperado → Export Carved File

8. Consultar log de auditoria
   └── Painel inferior → "FORENSIC AUDIT LOG"
       Botão "Export…" → salva log completo com hashes SHA-256
```

---

## 11. Solução de Problemas

### `UnsatisfiedLinkError: rdvc_jni` ao iniciar

- Verifique se `rdvc_core.dll` e `rdvc_jni.dll` estão na mesma pasta do JAR  
  **ou** passe `-Djava.library.path=build\output`
- No Linux: `export LD_LIBRARY_PATH=build/output:$LD_LIBRARY_PATH`

### `Access is denied` / `RDVC_ERR_DISK_ACCESS_DENIED`

- Execute o terminal / JAR **como Administrador** (Windows)
- No Linux: `sudo java ...` ou adicione o usuário ao grupo `disk`

### `JAVA_HOME is not set` durante o build JNI

```bat
set JAVA_HOME=C:\Program Files\Eclipse Adoptium\jdk-21.x.x.x-hotspot
```

```bash
export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
```

### `build\output\rdvc_core.dll not found` ao compilar JNI

Execute primeiro:
```bat
build\build-core-windows.bat
```

### Maven: `Plugin not found: javafx-maven-plugin`

Confirme que tem internet e que `mvn --version` retorna 3.9+.  
Na primeira execução o Maven baixa ~150 MB de dependências.

### `ClassNotFoundException: com.rdvc.App`

Use o fat JAR em `gui/target/` (não o JAR em `gui/target/original-*`).

---

## Variáveis de Ambiente Opcionais

| Variável | Efeito |
|---|---|
| `RDVC_LIBRARY_PATH` | Caminho explícito para `rdvc_jni.dll` / `librdvc_jni.so` |
| `JAVA_HOME` | Necessária para compilar a bridge JNI |
| `LD_LIBRARY_PATH` | Linux — pasta com as `.so` |

Exemplo de execução com caminho explícito:
```bash
java -DRDVC_LIBRARY_PATH=/opt/rdvc/lib \
     -jar raw-disk-viewer-cplx-1.0.0.jar
```
