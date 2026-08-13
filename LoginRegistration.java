import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class LoginRegistration extends JFrame {

    JTextField email, regEmail, dob, contact;
    JPasswordField password, regPassword;
    JPanel panel;

    LoginRegistration() {
        setTitle("Online Shopping - Login");
        setSize(400, 450);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        showLogin();
        setVisible(true);
    }

    void showLogin() {
        panel = new JPanel();
        panel.setLayout(new GridLayout(7, 1, 10, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(30, 30, 30, 30));

        JLabel title = new JLabel("LOGIN", SwingConstants.CENTER);
        title.setFont(new Font("Arial", Font.BOLD, 22));

        email = new JTextField();
        email.setBorder(BorderFactory.createTitledBorder("Email ID"));

        password = new JPasswordField();
        password.setBorder(BorderFactory.createTitledBorder("Password"));

        JButton login = new JButton("Login");
        JButton register = new JButton("New Registration");

        JLabel message = new JLabel("", SwingConstants.CENTER);

        login.addActionListener(e -> {
            if (email.getText().isEmpty() ||
                password.getPassword().length == 0) {
                message.setText("Invalid Email or Password!");
                message.setForeground(Color.RED);
            } else {
                message.setText("Login Successful!");
                message.setForeground(Color.GREEN);
            }
        });

        register.addActionListener(e -> showRegistration());

        panel.add(title);
        panel.add(email);
        panel.add(password);
        panel.add(login);
        panel.add(register);
        panel.add(message);

        add(panel);
        revalidate();
        repaint();
    }

    void showRegistration() {
        getContentPane().removeAll();

        panel = new JPanel();
        panel.setLayout(new GridLayout(9, 1, 8, 8));
        panel.setBorder(BorderFactory.createEmptyBorder(25, 30, 25, 30));

        JLabel title = new JLabel("NEW REGISTRATION", SwingConstants.CENTER);
        title.setFont(new Font("Arial", Font.BOLD, 20));

        regEmail = new JTextField();
        regEmail.setBorder(BorderFactory.createTitledBorder("Email ID"));

        regPassword = new JPasswordField();
        regPassword.setBorder(BorderFactory.createTitledBorder("Password"));

        dob = new JTextField();
        dob.setBorder(BorderFactory.createTitledBorder("Date of Birth"));

        contact = new JTextField();
        contact.setBorder(BorderFactory.createTitledBorder("Contact No"));

        JButton register = new JButton("Register");
        JButton back = new JButton("Back to Login");

        JLabel message = new JLabel("", SwingConstants.CENTER);

        register.addActionListener(e -> {
            String mail = regEmail.getText();
            String pass = new String(regPassword.getPassword());
            String date = dob.getText();
            String phone = contact.getText();

            if (mail.matches("^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$")
                    && pass.length() >= 6
                    && !date.isEmpty()
                    && phone.matches("[0-9]{10}")) {

                message.setText("Registration Successful!");
                message.setForeground(Color.GREEN);

            } else {
                message.setText("Invalid Details!");
                message.setForeground(Color.RED);
            }
        });

        back.addActionListener(e -> {
            getContentPane().removeAll();
            showLogin();
        });

        panel.add(title);
        panel.add(regEmail);
        panel.add(regPassword);
        panel.add(dob);
        panel.add(contact);
        panel.add(register);
        panel.add(back);
        panel.add(message);

        add(panel);
        revalidate();
        repaint();
    }

    public static void main(String[] args) {
        new LoginRegistration();
    }
}

