<div align="center">

```
██████╗  ██████╗ ██╗    ██╗     ██████╗ ██╗███████╗██╗  ██╗
██╔══██╗██╔══██╗██║    ██║     ██╔══██╗██║██╔════╝██║ ██╔╝
██████╔╝██║  ██║██║ █╗ ██║     ██║  ██║██║███████╗█████╔╝
██╔══██╗██║  ██║██║███╗██║     ██║  ██║██║╚════██║██╔═██╗
██║  ██║╚██████╔╝╚███╔███╔╝    ██████╔╝██║███████║██║  ██╗
╚═╝  ╚═╝ ╚═════╝  ╚══╝╚══╝     ╚═════╝ ╚═╝╚══════╝╚═╝  ╚═╝

 V I E W E R   C O M P L E X   ·   v 1 . 0 . 0
```

**Ferramenta forense de disco profissional (nível forense)**  
*Inspirado por FTK Imager / Autopsy · Implementado do zero em C + Java*

---

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-blue?style=for-the-badge)
![Java](https://img.shields.io/badge/Java-25-ED8B00?style=for-the-badge&logo=openjdk&logoColor=white)
![JavaFX](https://img.shields.io/badge/JavaFX-21-5382a1?style=for-the-badge)
![C](https://img.shields.io/badge/Language-C11-A8B9CC?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![JNI](https://img.shields.io/badge/Bridge-JNI-007396?style=for-the-badge)
![License](https://img.shields.io/badge/license-Educational%20%2F%20Forensic-green?style=for-the-badge)
![Version](https://img.shields.io/badge/version-1.0.0-blueviolet?style=for-the-badge)
![Build](https://img.shields.io/badge/build-MinGW--w64%20%7C%20GCC-orange?style=for-the-badge)

</div>

---

## O que é o RAW DISK VIEWER CPLX?

O **RAW DISK VIEWER CPLX** é uma aplicação completa de análise forense de discos, construída em três camadas: um motor nativo em C responsável pelo I/O bruto e parsing de partições, uma ponte JNI que expõe as funções nativas para Java, e uma interface GUI moderna em JavaFX.  

Permite a investigação de discos físicos e imagens no nível de byte: leitura hexadecimal em tempo real, análise de MBR/GPT, varredura por assinaturas de arquivos, cálculo de entropia por setor, recuperação (carving) e geração de logs forenses com integridade — tudo em **modo somente-leitura**.

---

## Sumário

- [Arquitetura](#arquitetura)
- [Funcionalidades](#funcionalidades)
- [Tecnologias](#tecnologias)
- [Estrutura do Projeto](#estrutura-do-projeto)
- [Pré-requisitos](#pré-requisitos)
- [Compilação](#compilação)
- [Execução](#execução)
- [Modo Demo](#modo-demo)
- [Como Funciona](#como-funciona)
- [Segurança e Projeto Forense](#segurança-e-projeto-forense)
- [Créditos](#créditos)
- [Licença](#licença)

---

## Arquitetura

O projeto é organizado em três camadas estritas, cada uma com responsabilidade definida:

```
┌─────────────────────────────────────────────────────────────────┐
│                 CAMADA 3 — GUI Java (JavaFX 21)                │
│  App.java · Launcher.java · MainWindow · HexCanvas · Painéis     │
├─────────────────────────────────────────────────────────────────┤
│                 CAMADA 2 — Ponte JNI (rdvc_jni.dll / .so)      │
│  bridge_disk.c · bridge_partition.c · bridge_scanner.c         │
├─────────────────────────────────────────────────────────────────┤
│                 CAMADA 1 — Core Nativo C (rdvc_core.dll / .so) │
│  disk_io.c · partition_mbr/gpt.c · signatures.c · carver.c     │
└─────────────────────────────────────────────────────────────────┘
```

Todos os acessos físicos são feitos em modo leitura. A camada C não possui código de escrita.

---

## Funcionalidades

- Visualização hexadecimal com canvas customizado e scroll virtual (16 bytes/linha).
- Parser completo de MBR (70+ tipos) e GPT (50+ GUIDs) com validação CRC32.
- Banco de assinaturas com 55+ tipos (JPEG, PNG, PDF, ZIP, EXE/PE, ELF, SQLite, E01, etc.).
- Cálculo de entropia (Shannon) por setor e classificação automática (EMPTY/TEXT/BINARY/COMPRESSED/ENCRYPTED).
- Motor de carving por janela deslizante para recuperação de arquivos.
- Bookmarks persistidos em JSON (cor, nota, LBA).
- Log forense com timestamp UTC e hash SHA-256 por linha (campo `integrity=`) para detecção de adulteração.
- Exportação de setores, exportação de arquivos recuperados e exportação de bookmarks.

---

## Tecnologias

| Camada | Tecnologia | Uso |
|---|---|---|
| Core nativo | C (C11) | I/O de baixo nível, parsing, carving, análise |
| Compatibilidade ABI | C++ (guards `extern "C"`) | Permite linkagem de projetos C++ sem name-mangling |
| API OS | Win32 / POSIX | Acesso físico (`CreateFile`, `ReadFile`, `open`, `ioctl`, etc.) |
| Ponte | JNI | Expõe funções nativas para Java |
| GUI | Java (OpenJDK 25) + JavaFX 21 | Interface, serviços, integração com NativeBridge |
| Serialização | GSON | Persistência de bookmarks em JSON |
| Build nativo | MinGW-w64 / GCC | Compilação em Windows e Linux |
| Build Java | Apache Maven 3.9+ | Compilação e empacotamento (Shade plugin) |

---

## Estrutura do Projeto

```
Raw Disk Viwer CPLX/
├── core/                # Código C nativo (headers + src)
├── jni/                 # Ponte JNI (bridge_*.c)
├── gui/                 # Aplicação Java/JavaFX + pom.xml
├── build/               # Scripts de build e saída (build/output)
├── docs/                # Documentação auxiliar
├── run.bat              # Launcher Windows (ajusta PATH e executa JAR)
└── README.md            # Este arquivo
```

---

## Pré-requisitos

### Compilação nativa (Core + JNI)

- Windows: MSYS2 + `mingw-w64-x86_64-toolchain` (GCC 12+).
- Linux: `build-essential` (GCC 12+).
- JDK 21+ com `JAVA_HOME` apontando para o JDK (necessário para headers JNI).

### GUI (Java)

- OpenJDK 25+
- Apache Maven 3.9+
- Acesso à Internet na primeira compilação (download de dependências Maven)

Verifique com:

```bash
gcc --version
java -version
mvn -version
echo $JAVA_HOME     # Linux/macOS
echo %JAVA_HOME%    # Windows PowerShell
```

---

## Compilação

Consulte o guia detalhado em `BUILD_GUIDE.md`. Resumo rápido:

### Windows

```bat
cd "Raw Disk Viwer CPLX"
build\build-core-windows.bat     REM compila rdvc_core.dll
build\build-jni-windows.bat      REM compila rdvc_jni.dll
cd gui && mvn -DskipTests package  REM gera raw-disk-viewer-cplx-1.0.0-shaded.jar
```

Após a compilação, `build\output\` conterá as DLLs nativas.

### Linux

```bash
chmod +x build/*.sh
./build/build-core-linux.sh
./build/build-jni-linux.sh
cd gui && mvn -DskipTests package
```

---

## Execução

> **Atenção:** A enumeração e leitura de discos físicos exige privilégios de administrador (Windows) ou root (Linux). Sem privilégios, a aplicação entra automaticamente em Modo Demo.

### Windows (recomendado — use `run.bat` como Administrador)

```bat
REM Clique com botão direito em run.bat → "Executar como administrador"
.\run.bat
```

O `run.bat` adiciona `build\output` ao `PATH` e define `-Djava.library.path` para que a JVM encontre as DLLs nativas.

### Execução manual (PowerShell)

```powershell
$lib = ".\build\output"
$jar = ".\gui\target\raw-disk-viewer-cplx-1.0.0-shaded.jar"
$env:PATH = "$lib;$env:PATH"
java --enable-preview "-Djava.library.path=$lib" -cp "$jar" com.rdvc.Launcher
```

### Linux

```bash
sudo java -Djava.library.path=./build/output -cp gui/target/raw-disk-viewer-cplx-1.0.0-shaded.jar com.rdvc.Launcher
```

### Fluxo inicial sugerido

1. Abra o aplicativo como Administrador/root.
2. Selecione um disco físico listado no painel esquerdo ou abra uma imagem (`.img`, `.dd`, `.raw`, `.iso`).
3. Navegue pelos setores no HexViewer (use o Sector Navigator para pular para um LBA específico).
4. Execute o scanner por assinaturas para localizar arquivos recuperáveis.
5. Adicione bookmarks em LBAs importantes; exporte setores ou arquivos recuperados conforme necessário.
6. Consulte o log forense (Forensic Log) para auditoria e cadeia de custódia.

---

## Modo Demo

Se as bibliotecas nativas (`rdvc_jni.dll`, `rdvc_core.dll` ou suas versões `.so`) não forem encontradas, a aplicação entra no **Modo Demo**:

- Dados sintéticos são gerados em memória para avaliar a interface e as funcionalidades.
- A maior parte da UI funciona normalmente para fins de desenvolvimento e demonstração.
- O título da janela exibe `[DEMO]` quando está em modo demo.

Para confirmar o carregamento nativo, verifique no log uma entrada como:

```
[...] INFO  NATIVE_LOAD  Biblioteca nativa carregada com sucesso
```

---

## Como Funciona

### Carregamento da biblioteca nativa (4 etapas)

`NativeLoader.java` tenta localizar/carregar a biblioteca JNI na seguinte ordem:

1. Propriedade Java: `-Drdvc.native.path=/caminho/absoluto/rdvc_jni.dll`
2. Diretório de trabalho: `./rdvc_jni.dll` (ao lado do JAR)
3. Recurso embutido no JAR: `/native/rdvc_jni.dll`
4. `java.library.path` / pesquisa do sistema

### Enumeração de discos (Windows — resumo)

O core nativo `rdvc_list_disks()` tenta abrir `\\.\\PhysicalDrive0` a `\\.\\PhysicalDrive31` e, para cada handle válido, obtém geometria (via `IOCTL_DISK_GET_DRIVE_GEOMETRY_EX`) e modelo (via `IOCTL_STORAGE_QUERY_PROPERTY`).

Trecho ilustrativo (C):

```c
// disk_io.c — rdvc_list_disks()
for (u32 i = 0; i < 32; i++) {
    snprintf(path, sizeof(path), "\\\\.\\PhysicalDrive%u", i);
    if (rdvc_platform_open(path, &h) == RDVC_OK) {
        // obter geometria e modelo; preencher rdvc_disk_info_t
    }
}
```

### Entropia de Shannon (resumo)

Fórmula:

$$H(X) = -\sum p(x) \cdot \log_2 p(x)$$

Interpretação prática usada pelo motor:

- H < 0.5  →  EMPTY (setor vazio)
- H < 5.0  →  TEXT / baixa entropia
- H < 7.0  →  BINARY / dados comuns ou comprimidos
- H ≥ 7.0  →  ENCRYPTED / dados aleatórios

### Formato do log forense

Exemplo de linha:

```
[2026-04-04T15:32:10.441Z] INFO  DISK_OPEN  path=\\.\\PhysicalDrive0 handle=0x1a4  integrity=f3a82c91
```

Cada linha termina com `integrity=` contendo os primeiros 8 bytes hex do SHA-256 da linha para verificação rápida de integridade.

---

## Segurança e Projeto Forense

| Garantia | Como é implementada |
|---|---|
| Acesso somente-leitura | Uso de `GENERIC_READ` / `O_RDONLY` no core nativo; não existe caminho de escrita |
| Sem ferramentas externas | Scanner e carver funcionam internamente em C; nenhum `system()`/`exec()` é usado |
| Log com prova de integridade | SHA-256 por linha no log forense |
| Privacidade | Não são registrados dados pessoais — apenas LBAs, handles e tipos de arquivo |
| Requisitos de privilégio | Leitura de discos brutos respeita controles do SO (admin/root) |

---

## Créditos

<div align="center">

### Desenvolvido por Escanearcpl

**Software Developer · Pesquisador de Segurança · Entusiasta de Ferramentas Forenses**  
Brasil 🇧🇷

---

*"Cada byte conta uma história. Esta ferramenta ajuda a lê-la."*

---

> Projeto idealizado e arquitetado por Escanearcpl com contribuições, testes e suporte.  
> Implementação completa: core C, ponte JNI e interface JavaFX.

</div>

---

## Licença

Este projeto é disponibilizado para **fins educacionais e pesquisa forense**. Entre em contato com o autor antes de redistribuir ou usar em ambientes forenses de produção.

---

<div align="center">

**RAW DISK VIEWER CPLX** · v1.0.0 · © 2026 Henry  

*C · C++ ABI · JNI · Java 25 · JavaFX 21 · CSS · XML · Batch · Bash · MinGW-w64 · Maven · Win32 · POSIX*

</div>
