public class InspectMain {
    public static void main(String[] args) throws Exception {
        Class<?> c = Class.forName("com.rdvc.App");
        java.lang.reflect.Method m = c.getMethod("main", String[].class);
        System.out.println("Return type: " + m.getReturnType().getName());
        System.out.println("Modifiers: " + java.lang.reflect.Modifier.toString(m.getModifiers()));
        System.out.println("IsSynthetic: " + m.isSynthetic());
    }
}
