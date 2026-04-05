package com.rdvc;

import com.rdvc.bridge.NativeLoader;
import com.rdvc.ui.MainWindow;
import com.rdvc.forensic.ForensicLogger;
import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

/**
 * App — JavaFX application entry point for RAW DISK VIEWER CPLX.
 */
public class App extends Application {

    public static final String APP_NAME    = "RAW DISK VIEWER CPLX";
    public static final String APP_VERSION = "1.0.0";
    public static final String APP_BUILD   = "2026.04";

    @Override
    public void init() {
        // Load native library before any window is shown
        try {
            NativeLoader.load();
            ForensicLogger.global().info("STARTUP",
                APP_NAME + " v" + APP_VERSION + " starting");
        } catch (UnsatisfiedLinkError e) {
            ForensicLogger.global().warn("STARTUP",
                "Native library not loaded — demo mode active. " + e.getMessage());
        }
    }

    @Override
    public void start(Stage primaryStage) {
        MainWindow mainWindow = new MainWindow(primaryStage);
        Scene scene = new Scene(mainWindow, 1400, 900);

        // Load dark theme CSS
        try {
            String css = getClass().getResource("/css/dark-theme.css").toExternalForm();
            scene.getStylesheets().add(css);
        } catch (Exception e) {
            System.err.println("Warning: dark-theme.css not found, using default style.");
        }

        primaryStage.setTitle(APP_NAME + " v" + APP_VERSION + "  [READ-ONLY]");
        primaryStage.setScene(scene);
        primaryStage.setMinWidth(1000);
        primaryStage.setMinHeight(700);
        primaryStage.setOnCloseRequest(e -> mainWindow.shutdown());
        primaryStage.show();

        ForensicLogger.global().info("STARTUP", "GUI initialized");
    }

    @Override
    public void stop() {
        ForensicLogger.global().info("SHUTDOWN", APP_NAME + " closing");
        ForensicLogger.global().flush();
    }

    public static void main(String[] args) {
        launch(args);
    }
}
