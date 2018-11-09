width = 80
text = input("Enter text: ")
text = ' ' + text + ' '
tabs = int(input("Enter number of tabs: "))
bulk = '\t'*tabs + (width - 4*tabs)*'/'
left = (width - len(text)) // 2
print(bulk)
print(bulk[:left] + text + bulk[left+len(text):])
print(bulk)
