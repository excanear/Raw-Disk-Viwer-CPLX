package com.rdvc;

import javafx.application.Application;

/**
 * Launcher wrapper to avoid JVM/launcher main-signature issues when launching JavaFX.
 */
public class Launcher {
    public static void main(String[] args) {
        Application.launch(App.class, args);
    }
}
