# git 常用命令
## stash
|||
|:-|:-|
git stash push [*\<msg>*]|保存当前修改到新的存档库，并且执行git reset ‑‑hard来回滚. msg是可选的来描述存档
git stash pop|应用存档库最后一个（或指定的）改动，然后从存档库丢弃它
git stash apply [*\<stash>*]|将存档中某个修改（默认是最近的）应用到工作区
git stash list|显示当前你有的所有存档
git stash show [*\<stash>*]|显示存档中记录的改动，对比存档生成时的原来状态；不指定stash则显示最后一个
git stash drop [*\<stash>*]|从存档库删除单个修改；不指定stash则删除最后一个
git stash clear|清空存档库。注意相关存档会被清理，此操作strong不能被恢复/strong
git stash branch \<branchname> [*\<stash>*]|新建并检出一个新分支branchname, 分支开始于存档建立时的源提交，应用存档的变化作为新的工作区和暂存区。如果成功并且stash是以 stash@{revision}方式给出的，则从存档库删除它。未给出则使用最后一个存档。这在当前分支运行 stash push 导致冲突时很好用，因为存档应用于它生成时的提交一定不会有冲突发生

## workspace
|||
|:-|:-|
git status|显示状态变化，包括1)暂存区与当前的 HEAD 提交之间(即将提交的)，2）工作区与暂存区(下次不会提交)，3）未曾被git追踪 (没有历史记录)
git diff|显示未添加到暂存区的不同
git diff *commit or branch*|查看工作区与某一提交之间的不同。你也可以使用 HEAD 来对比上一提交，或是用分支 名来和分支比较
git add *file... or dir...*|添加当前的新内容或是修改的文件到暂存区，作为下次提交的(部分)内容。用add -- interactive 来交互式操作
git add -u|添加当前修改到暂存区, 这与'git commit -a'准备提交内容的方式一致
git rm *\<file(s)...>*|从工作区和暂存区删除某个文件
git mv *\<file(s)...>*|从工作区和暂存区移动文件
git commit -a [-m 'msg']|提交上次提交之后的所有修改，1)未追踪的除外(即：所有暂存区有记录的文件)；2)从暂存区删除已在工作区删除的文件
git checkout *\<files(s)... or dir>*|更新工作区文件或文件夹，不会切换分支
git reset --hard|恢复工作区和暂存区到上次提交的状态，警告： 所有工作区修改都会被丢弃。使用这条命令来解决合并错误，如果你想从头开始的话传入 ORIG_HEAD 来撤销该次提交以来的所有改动
git reset --hard *\<remote>/\<branch>*|重置本地版本库，让它与远程版本一致；用 reset ‑‑hard origin/master 来丢弃所有的本地改动；用这个来处理失败的合并，直接从远程开始
git switch *\<branch>*|切换分支，更改工作区和暂存区为 branch 分支的内容，之后HEAD指向 branch分支
git checkout -b *\<name of new branch>*|新建一个分支并且立即切换过去
git merge *\<commit or branch>*|从 branch name 分支合并到当前分支，使用‑‑no-commit可以保持在(已经合并)但未提交状态
git rebase *\<upstream>*|衍合：回滚从【当前提交和 upstream 分支分开处】开始直到当前提交的所有提交，将这些提交一一应用到 upstream 分支，结果作为 upstream 的新提交Reverts all commits since the current branch diverged from upstream , and then reapplies them one-by-one on top of changes from the HEAD of upstream .
git cherry-pick *\<commit>*|把某个提交移动到当前分支来
git revert *\<commit>*|回滚 commit 指定的提交，这需要当前工作区是干净的，即相比于 HEAD 提交没有修改
git clone *\<repo>*|下载repo指定的版本库，并在工作区迁出master分支的HEAD版本
git pull *\<remote> \<refspec>*|从远程版本库取得修改到当前分支. 一般来说, git pull 相当于 git fetch 然后做 git merge FETCH_HEAD.
git clean|
## index
|||
|:-|:-|
git reset HEAD *\<file(s)...>*|从下次提交中移除指定文件。重置暂存区记录但是不处理工作区(即: 文件改动被保留但不会被提交)，同时报告没有被更新的文件
git reset --soft HEAD^|恢复上一次提交，保留暂存区的改动
git diff --cached [*\<commit>*]|查看已经暂存的内容和上次提交的区别，也可指定某一提交
git commit -m 'msg'|暂存区中的当前内容连同提交信息储存为新提交
git commit --amend|用当前暂存去的内容修改最近一次的提交，也可以拿来修改提交信息
## local repo
|||
|:-|:-|
git log|显示最近的提交，新的在上边。参数:‑‑decorate显示分支和tag名字到对应的提交；‑‑stat显示状态 (文件修改, 添加, 删除)；‑‑author= author 只显示某个作者；‑‑after="MMM DD YYYY" 如("Jun 20 2008") 只显示某个日期之后的提交；‑‑before="MMM DD YYYY" 只显示某个日期之前的提交；‑‑merge 只与当前合并冲突有关的提交；--reverse：时间由远及近
git diff *\<commit> \<commit>*|显示两个提交之间的不同
git branch|显示所有（本地）存在的分支。参数 -r 显示远程追踪分支，参数 -a 显示全部
git branch -d *\<branch>*|删除某个分支，使用—D来强制删除
git branch --track *\<new> \<remote/branch>*|添加一个本地分支来跟踪某个远程分支
git fetch *\<remote> \<refspec>*|从远端版本库下载对象和引用(即版本信息)
git push|从本地提交推送分支改变到远程，分支为所有推送过的分支
git push *\<remote> \<branch>*|向远端版本库推送新的(已存在的)分支
git push *\<remote> \<branch>:\<branch>*|向远端版本库推送分支，但是从不同的（本地）分支名
git push *\<remote> \<branch>:\<branch>*|删除一个远程分支，通过向远程分支推送空内容







