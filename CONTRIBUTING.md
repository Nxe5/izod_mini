# Contributing to Izod Mini

Thank you for your interest in contributing to the Izod Mini project! This document provides guidelines and information for contributors.

## 🎯 How to Contribute

### Reporting Issues
- Use the GitHub issue templates for bug reports and feature requests
- Provide detailed information about your environment and steps to reproduce
- Include relevant logs, screenshots, or error messages

### Suggesting Features
- Check existing issues to avoid duplicates
- Provide clear use cases and benefits
- Consider implementation complexity and impact

### Code Contributions
- Fork the repository and create a feature branch
- Follow the coding standards and conventions
- Add tests where applicable
- Update documentation as needed
- Submit a pull request with a clear description

## 🏗️ Development Setup

### Hardware Development
- **KiCad**: Use KiCad 7.0+ for PCB design
- **Version Control**: Commit KiCad files in their native format
- **Documentation**: Update BOMs and assembly guides when making changes

### Firmware Development
- **ESP-IDF**: Recommended for ESP32 development
- **Arduino IDE**: Alternative option with ESP32 board support
- **Code Style**: Follow ESP-IDF coding standards
- **Testing**: Test on actual hardware when possible

### Mechanical Design
- **File Formats**: Provide both source files and export formats (STL, STEP)
- **Documentation**: Include assembly instructions and tolerances
- **Renders**: Add 3D renders for visual documentation

## 📋 Pull Request Process

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### PR Guidelines
- Use descriptive titles and descriptions
- Reference related issues
- Include screenshots or diagrams for UI/mechanical changes
- Ensure all checks pass
- Request reviews from maintainers

## 🏷️ Release Process

### Versioning
We follow [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: New functionality in a backwards compatible manner
- **PATCH**: Backwards compatible bug fixes

### Signing
- Releases are automatically signed using GitHub Actions
- Tags trigger the release process
- Private keys are stored in GitHub Secrets

## 📝 Code of Conduct

### Our Pledge
We are committed to providing a welcoming and inclusive environment for all contributors.

### Expected Behavior
- Use welcoming and inclusive language
- Be respectful of differing viewpoints and experiences
- Accept constructive criticism gracefully
- Focus on what is best for the community
- Show empathy towards other community members

### Unacceptable Behavior
- Harassment, trolling, or inflammatory comments
- Personal attacks or political discussions
- Public or private harassment
- Publishing private information without permission
- Other unprofessional conduct

## 🔧 Development Guidelines

### Hardware Design
- Follow KiCad best practices
- Use standard component footprints
- Include proper documentation
- Consider manufacturability and cost
- Test designs before submission

### Firmware Development
- Write clean, readable code
- Add comments for complex logic
- Follow ESP-IDF conventions
- Optimize for power consumption
- Include error handling

### Documentation
- Write clear, concise documentation
- Include examples and code snippets
- Keep documentation up to date
- Use proper markdown formatting
- Add diagrams where helpful

## 🐛 Bug Reports

When reporting bugs, please include:

1. **Environment**: OS, software versions, hardware revision
2. **Steps to Reproduce**: Detailed steps to reproduce the issue
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Additional Context**: Logs, screenshots, or other relevant information

## 💡 Feature Requests

When suggesting features, please include:

1. **Use Case**: Why is this feature needed?
2. **Proposed Solution**: How should it work?
3. **Alternatives**: Other solutions you've considered
4. **Additional Context**: Any other relevant information

## 📞 Getting Help

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For general questions and community chat
- **Documentation**: Check the `docs/` folder for guides

## 🏆 Recognition

Contributors will be recognized in:
- CONTRIBUTORS.md file
- Release notes
- Project documentation

Thank you for contributing to Izod Mini! 🎵

